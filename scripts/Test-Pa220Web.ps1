[CmdletBinding()]
param([string]$Target="192.168.2.2",[int]$Port=80,[int]$TimeoutMs=10000,[string]$LogPath=".\pa220-web-test.log")
$ErrorActionPreference="Stop"
$results=New-Object System.Collections.Generic.List[string]
function Write-Result([string]$Message) {
  $line="{0} {1}" -f (Get-Date -Format "yyyy-MM-ddTHH:mm:ss.fffK"),$Message
  $results.Add($line); Write-Host $line
}
function Invoke-Pa220HttpCase([string]$Method,[string]$Path,[int]$ExpectedStatus) {
  $client=New-Object System.Net.Sockets.TcpClient
  try {
    $async=$client.BeginConnect($Target,$Port,$null,$null)
    if(-not $async.AsyncWaitHandle.WaitOne($TimeoutMs)){throw "CONNECT_TIMEOUT target=$Target port=$Port"}
    $client.EndConnect($async); $client.ReceiveTimeout=$TimeoutMs; $client.SendTimeout=$TimeoutMs
    $stream=$client.GetStream()
    $crlf=[string]([char]13)+[char]10
    $request="$Method $Path HTTP/1.0"+$crlf+"Host: pa220"+$crlf+"Connection: close"+$crlf+$crlf
    $bytes=[Text.Encoding]::ASCII.GetBytes($request); $stream.Write($bytes,0,$bytes.Length); $stream.Flush()
    $client.Client.Shutdown([Net.Sockets.SocketShutdown]::Send)
    $memory=New-Object IO.MemoryStream; $buffer=New-Object byte[] 4096
    while($true){$n=$stream.Read($buffer,0,$buffer.Length);if($n -eq 0){break};$memory.Write($buffer,0,$n);if($memory.Length -gt 65536){throw "RESPONSE_LIMIT_EXCEEDED"}}
    $raw=[Text.Encoding]::ASCII.GetString($memory.ToArray());$split=$raw.IndexOf($crlf+$crlf)
    if($split -lt 0){throw "MISSING_HTTP_HEADER_TERMINATOR path=$Path"}
    $head=$raw.Substring(0,$split);$body=$raw.Substring($split+4)
    if($head -notmatch '^HTTP/1\.[01] ([0-9]{3}) '){throw "INVALID_STATUS_LINE: $head"}
    $actual=[int]$Matches[1]
    if($actual -ne $ExpectedStatus){throw "STATUS_MISMATCH path=$Path expected=$ExpectedStatus actual=$actual response=$raw"}
    if($head -notmatch '(?im)^Content-Length:\s*([0-9]+)\s*$'){throw "MISSING_CONTENT_LENGTH path=$Path"}
    $expectedLength=[int]$Matches[1];$actualLength=[Text.Encoding]::ASCII.GetByteCount($body)
    if($actualLength -ne $expectedLength){throw "BODY_LENGTH_MISMATCH path=$Path expected=$expectedLength actual=$actualLength"}
    if($ExpectedStatus -eq 200 -and  $Path -eq "/api/board" -and  $body -notmatch '"board"\s*:\s*"Palo Alto Networks PA-220"'){throw "BOARD_JSON_MISMATCH: $body"}
    Write-Result "PASS method=$Method path=$Path status=$actual body_bytes=$actualLength eof=1"
    return $raw
  } finally {if($client){$client.Close()}}
}
try {
  Write-Result "START target=$Target port=$Port timeout_ms=$TimeoutMs"
  Write-Result "NOTE foreground pa220 web must already be running; no boot/reset/config/flash operations are performed."
  [void](Invoke-Pa220HttpCase "GET" "/api/board" 200)
  [void](Invoke-Pa220HttpCase "GET" "/" 200)
  [void](Invoke-Pa220HttpCase "GET" "/api/status" 200)
  [void](Invoke-Pa220HttpCase "GET" "/api/memory" 200)
  [void](Invoke-Pa220HttpCase "GET" "/api/network" 200)
  [void](Invoke-Pa220HttpCase "GET" "/not-found" 404)
  [void](Invoke-Pa220HttpCase "POST" "/api/status" 405)
  $abort=New-Object Net.Sockets.TcpClient
  $async=$abort.BeginConnect($Target,$Port,$null,$null)
  if(-not $async.AsyncWaitHandle.WaitOne($TimeoutMs)){throw "RST_TEST_CONNECT_TIMEOUT"}
  $abort.EndConnect($async);$abort.Client.LingerState=[System.Net.Sockets.LingerOption]::new($true,0);$abort.Close()
  Start-Sleep -Milliseconds 500;Write-Result "RST_ABORT_SENT"
  [void](Invoke-Pa220HttpCase "GET" "/api/status" 200)
  Write-Result "SUMMARY PASS cases=8 rst_recovery=PASS"
} catch {
  Write-Result ("FAIL "+$_.Exception.Message)
  $results|Set-Content -Encoding UTF8 $LogPath
  throw
} finally {
  if($results.Count -gt 0){$results|Set-Content -Encoding UTF8 $LogPath}
}