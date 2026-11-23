[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$PcapPath,[string]$Target="192.168.2.2",[int]$Port=80,[string]$CsvPath=".\pa220-web-packets.csv")
$ErrorActionPreference="Stop"
$tshark=(Get-Command tshark -ErrorAction Stop).Source
if(-not(Test-Path -LiteralPath $PcapPath -PathType Leaf)){throw "PCAP_NOT_FOUND: $PcapPath"}
$filter="ip.addr == $Target && tcp.port == $Port"
$args=@("-r",$PcapPath,"-Y",$filter,"-T","fields","-E","header=y","-E","separator=,","-E","quote=d",
"-e","frame.number","-e","frame.time_epoch","-e","eth.src","-e","eth.dst","-e","ip.src","-e","tcp.srcport",
"-e","ip.dst","-e","tcp.dstport","-e","tcp.flags","-e","tcp.seq","-e","tcp.ack","-e","tcp.len","-e","tcp.stream",
"-e","http.request.method","-e","http.request.uri","-e","http.response.code")
& $tshark @args | Set-Content -Encoding UTF8 $CsvPath
if($LASTEXITCODE-ne 0){throw "TSHARK_EXPORT_FAILED exit=$LASTEXITCODE"}
Write-Host "CSV=$((Resolve-Path $CsvPath).Path)"
Write-Host "FILTER=$filter"
Write-Host "Flags: SYN=0x0002 SYN-ACK=0x0012 FIN+ACK=0x0011 ACK=0x0010 RST=0x0004"
Write-Host "Verify request/response payload, ACK progression, server FIN after all response ACKs, and final ACK."