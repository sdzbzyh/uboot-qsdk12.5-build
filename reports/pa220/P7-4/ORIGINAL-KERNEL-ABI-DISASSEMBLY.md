# PA-220 original PAN-OS kernel: boot ABI disassembly

Read-only disassembly of the exact user-exported kernel. No device interaction or persistent writes.

- File: artifacts/pa220/original-kernel/pa220-original-vmlinux
- Size: 10387920 bytes
- SHA-256: eff6e9d16cf11144642404ddd010bb9d1f62616df185460491a9cd1fd7639e6d
- Build ID: b305d1bd17f2baaf2776ace2712707b47a73a3ae
- Tool: GNU objdump (GNU Binutils for Ubuntu) 2.46

## ELF header and program headers

~~~text
ELF Header:
  Magic:   7f 45 4c 46 02 02 01 00 00 00 00 00 00 00 00 00 
  Class:                             ELF64
  Data:                              2's complement, big endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              EXEC (Executable file)
  Machine:                           MIPS R3000
  Version:                           0x1
  Entry point address:               0xffffffff80623fa0
  Start of program headers:          64 (bytes into file)
  Start of section headers:          10386192 (bytes into file)
  Flags:                             0x808d0001, noreorder, octeon2, mips64r2
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         2
  Size of section headers:           64 (bytes)
  Number of section headers:         27
  Section header string table index: 26

Elf file type is EXEC (Executable file)
Entry point 0xffffffff80623fa0
There are 2 program headers, starting at offset 64

Program Headers:
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
  ABIFLAGS       0x64cd68 0xffffffff8074bd68 0xffffffff8074bd68 0x000018 0x000018 R   0x8
  LOAD           0x001000 0xffffffff80100000 0xffffffff80100000 0x7b0a80 0x852640 RWE 0x1000

 Section to Segment mapping:
  Segment Sections...
   00     .MIPS.abiflags 
   01     .text __ex_table .notes .rodata .MIPS.abiflags .pci_fixup __ksymtab __ksymtab_gpl __kcrctab __kcrctab_gpl __ksymtab_strings __param __modver .data .init.text .init.data .exit.text .data..percpu .bss 
~~~

## Relevant symbols

~~~text
ffffffff80623fa0 0000000000000314 T kernel_entry
ffffffff80866f74 00000000000009c8 T prom_init
ffffffff8086793c 0000000000000334 T plat_mem_setup
ffffffff80867f40 000000000000013c T device_tree_init
ffffffff8086af80 0000000000000028 T octeon_setup_smp
ffffffff808e7d68 0000000000000008 B octeon_bootinfo
ffffffff808e7d70 0000000000000008 B octeon_boot_desc_ptr
~~~

## Disassembly: prom_init

~~~asm

artifacts/pa220/original-kernel/pa220-original-vmlinux:     file format elf64-tradbigmips


Disassembly of section .text:

Disassembly of section .init.text:

ffffffff80866f74 <prom_init>:
ffffffff80866f74:	3c02808f 	lui	v0,0x808f
ffffffff80866f78:	67bdff70 	daddiu	sp,sp,-144
ffffffff80866f7c:	dc44afd0 	ld	a0,-20528(v0)
ffffffff80866f80:	3c038016 	lui	v1,0x8016
ffffffff80866f84:	ffbf0088 	sd	ra,136(sp)
ffffffff80866f88:	6463a6b8 	daddiu	v1,v1,-22856
ffffffff80866f8c:	ffb40060 	sd	s4,96(sp)
ffffffff80866f90:	3c05808f 	lui	a1,0x808f
ffffffff80866f94:	ffb30058 	sd	s3,88(sp)
ffffffff80866f98:	3c13808e 	lui	s3,0x808e
ffffffff80866f9c:	ffb20050 	sd	s2,80(sp)
ffffffff80866fa0:	3c14808e 	lui	s4,0x808e
ffffffff80866fa4:	ffb10048 	sd	s1,72(sp)
ffffffff80866fa8:	2411ffff 	li	s1,-1
ffffffff80866fac:	ffb00040 	sd	s0,64(sp)
ffffffff80866fb0:	00118ffc 	dsll32	s1,s1,0x1f
ffffffff80866fb4:	ffbe0080 	sd	s8,128(sp)
ffffffff80866fb8:	02809025 	move	s2,s4
ffffffff80866fbc:	ffb70078 	sd	s7,120(sp)
ffffffff80866fc0:	ffb60070 	sd	s6,112(sp)
ffffffff80866fc4:	ffb50068 	sd	s5,104(sp)
ffffffff80866fc8:	dc820188 	ld	v0,392(a0)
ffffffff80866fcc:	fe647d70 	sd	a0,32112(s3)
ffffffff80866fd0:	fca3d180 	sd	v1,-11904(a1)
ffffffff80866fd4:	0051102d 	daddu	v0,v0,s1
ffffffff80866fd8:	9c44003c 	lwu	a0,60(v0)
ffffffff80866fdc:	0c040446 	jal	ffffffff80101118 <cvmx_bootmem_init>
ffffffff80866fe0:	fe827d68 	sd	v0,32104(s4)
ffffffff80866fe4:	0c0415f4 	jal	ffffffff801057d0 <cvmx_sysinfo_get>
ffffffff80866fe8:	00000000 	nop
ffffffff80866fec:	00002825 	move	a1,zero
ffffffff80866ff0:	24060130 	li	a2,304
ffffffff80866ff4:	00402025 	move	a0,v0
ffffffff80866ff8:	0c0e4e40 	jal	ffffffff80393900 <memset>
ffffffff80866ffc:	00408025 	move	s0,v0
ffffffff80867000:	de827d68 	ld	v0,32104(s4)
ffffffff80867004:	8c420038 	lw	v0,56(v0)
ffffffff80867008:	70425d32 	cins	v0,v0,0x14,0xb
ffffffff8086700c:	fe020000 	sd	v0,0(s0)
ffffffff80867010:	de857d68 	ld	a1,32104(s4)
ffffffff80867014:	9ca2003c 	lwu	v0,60(a1)
ffffffff80867018:	0051882d 	daddu	s1,v0,s1
ffffffff8086701c:	fe110008 	sd	s1,8(s0)
ffffffff80867020:	8ca20000 	lw	v0,0(a1)
ffffffff80867024:	2c430002 	sltiu	v1,v0,2
ffffffff80867028:	10600008 	beqz	v1,ffffffff8086704c <prom_init+0xd8>
ffffffff8086702c:	24060080 	li	a2,128
ffffffff80867030:	24030001 	li	v1,1
ffffffff80867034:	1443000a 	bne	v0,v1,ffffffff80867060 <prom_init+0xec>
ffffffff80867038:	00000000 	nop
ffffffff8086703c:	8ca20004 	lw	v0,4(a1)
ffffffff80867040:	2c420004 	sltiu	v0,v0,4
ffffffff80867044:	14400006 	bnez	v0,ffffffff80867060 <prom_init+0xec>
ffffffff80867048:	00000000 	nop
ffffffff8086704c:	64a500b8 	daddiu	a1,a1,184
ffffffff80867050:	0c0588d0 	jal	ffffffff80162340 <memcpy>
ffffffff80867054:	66040028 	daddiu	a0,s0,40
ffffffff80867058:	10000004 	b	ffffffff8086706c <prom_init+0xf8>
ffffffff8086705c:	3c03010d 	lui	v1,0x10d
ffffffff80867060:	9ca20034 	lwu	v0,52(a1)
ffffffff80867064:	fe020028 	sd	v0,40(s0)
ffffffff80867068:	3c03010d 	lui	v1,0x10d
ffffffff8086706c:	34639508 	ori	v1,v1,0x9508
ffffffff80867070:	40027800 	mfc0	v0,$15
ffffffff80867074:	00431026 	xor	v0,v0,v1
ffffffff80867078:	3c0300ff 	lui	v1,0xff
ffffffff8086707c:	3463ff00 	ori	v1,v1,0xff00
ffffffff80867080:	00431024 	and	v0,v0,v1
ffffffff80867084:	1040000f 	beqz	v0,ffffffff808670c4 <prom_init+0x150>
ffffffff80867088:	de427d68 	ld	v0,32104(s2)
ffffffff8086708c:	24030200 	li	v1,512
ffffffff80867090:	24070001 	li	a3,1
ffffffff80867094:	24060400 	li	a2,1024
ffffffff80867098:	000311bb 	dsra	v0,v1,0x6
ffffffff8086709c:	00672014 	dsllv	a0,a3,v1
ffffffff808670a0:	000210f8 	dsll	v0,v0,0x3
ffffffff808670a4:	00042827 	nor	a1,zero,a0
ffffffff808670a8:	0202102d 	daddu	v0,s0,v0
ffffffff808670ac:	24630001 	addiu	v1,v1,1
ffffffff808670b0:	dc440028 	ld	a0,40(v0)
ffffffff808670b4:	00852024 	and	a0,a0,a1
ffffffff808670b8:	1466fff7 	bne	v1,a2,ffffffff80867098 <prom_init+0x124>
ffffffff808670bc:	fc440028 	sd	a0,40(v0)
ffffffff808670c0:	de427d68 	ld	v0,32104(s2)
ffffffff808670c4:	24060014 	li	a2,20
ffffffff808670c8:	66040113 	daddiu	a0,s0,275
ffffffff808670cc:	9c430028 	lwu	v1,40(v0)
ffffffff808670d0:	6445009b 	daddiu	a1,v0,155
ffffffff808670d4:	fe0300b0 	sd	v1,176(s0)
ffffffff808670d8:	8c430044 	lw	v1,68(v0)
ffffffff808670dc:	ae0300b8 	sw	v1,184(s0)
ffffffff808670e0:	8c430048 	lw	v1,72(v0)
ffffffff808670e4:	00031840 	sll	v1,v1,0x1
ffffffff808670e8:	ae0300bc 	sw	v1,188(s0)
ffffffff808670ec:	94430050 	lhu	v1,80(v0)
ffffffff808670f0:	a60300c0 	sh	v1,192(s0)
ffffffff808670f4:	90430098 	lbu	v1,152(v0)
ffffffff808670f8:	a2030110 	sb	v1,272(s0)
ffffffff808670fc:	90430099 	lbu	v1,153(v0)
ffffffff80867100:	a2030111 	sb	v1,273(s0)
ffffffff80867104:	9042009a 	lbu	v0,154(v0)
ffffffff80867108:	0c0588d0 	jal	ffffffff80162340 <memcpy>
ffffffff8086710c:	a2020112 	sb	v0,274(s0)
ffffffff80867110:	de427d68 	ld	v0,32104(s2)
ffffffff80867114:	24060006 	li	a2,6
ffffffff80867118:	660400c4 	daddiu	a0,s0,196
ffffffff8086711c:	90430052 	lbu	v1,82(v0)
ffffffff80867120:	6445006c 	daddiu	a1,v0,108
ffffffff80867124:	a20300c2 	sb	v1,194(s0)
ffffffff80867128:	90420053 	lbu	v0,83(v0)
ffffffff8086712c:	0c0588d0 	jal	ffffffff80162340 <memcpy>
ffffffff80867130:	a20200c3 	sb	v0,195(s0)
ffffffff80867134:	de457d68 	ld	a1,32104(s2)
ffffffff80867138:	24060014 	li	a2,20
ffffffff8086713c:	660400cb 	daddiu	a0,s0,203
ffffffff80867140:	90a20072 	lbu	v0,114(a1)
ffffffff80867144:	64a50058 	daddiu	a1,a1,88
ffffffff80867148:	0c0588d0 	jal	ffffffff80162340 <memcpy>
ffffffff8086714c:	a20200ca 	sb	v0,202(s0)
ffffffff80867150:	de427d68 	ld	v0,32104(s2)
ffffffff80867154:	3c03807c 	lui	v1,0x807c
ffffffff80867158:	2405004a 	li	a1,74
ffffffff8086715c:	6463e880 	daddiu	v1,v1,-6016
ffffffff80867160:	dc460078 	ld	a2,120(v0)
ffffffff80867164:	fe0600e0 	sd	a2,224(s0)
ffffffff80867168:	dc460080 	ld	a2,128(v0)
ffffffff8086716c:	fe0600e8 	sd	a2,232(s0)
ffffffff80867170:	dc460088 	ld	a2,136(v0)
ffffffff80867174:	fe0600f0 	sd	a2,240(s0)
ffffffff80867178:	8c460090 	lw	a2,144(v0)
ffffffff8086717c:	ae0600f8 	sw	a2,248(s0)
ffffffff80867180:	8c420094 	lw	v0,148(v0)
ffffffff80867184:	ae0200fc 	sw	v0,252(s0)
ffffffff80867188:	9f820020 	lwu	v0,32(gp)
ffffffff8086718c:	00021238 	dsll	v0,v0,0x8
ffffffff80867190:	0062102d 	daddu	v0,v1,v0
ffffffff80867194:	8c420024 	lw	v0,36(v0)
ffffffff80867198:	14450005 	bne	v0,a1,ffffffff808671b0 <prom_init+0x23c>
ffffffff8086719c:	3c04808e 	lui	a0,0x808e
ffffffff808671a0:	3c02fff0 	lui	v0,0xfff0
ffffffff808671a4:	64420023 	daddiu	v0,v0,35
ffffffff808671a8:	10000008 	b	ffffffff808671cc <prom_init+0x258>
ffffffff808671ac:	000212fc 	dsll32	v0,v0,0xb
ffffffff808671b0:	2405004b 	li	a1,75
ffffffff808671b4:	1445000c 	bne	v0,a1,ffffffff808671e8 <prom_init+0x274>
ffffffff808671b8:	3c02fff0 	lui	v0,0xfff0
ffffffff808671bc:	64420023 	daddiu	v0,v0,35
ffffffff808671c0:	000214b8 	dsll	v0,v0,0x12
ffffffff808671c4:	64420003 	daddiu	v0,v0,3
ffffffff808671c8:	00021678 	dsll	v0,v0,0x19
ffffffff808671cc:	dc421600 	ld	v0,5632(v0)
ffffffff808671d0:	7c452e03 	dext	a1,v0,0x18,0x6
ffffffff808671d4:	3c0202fa 	lui	v0,0x2fa
ffffffff808671d8:	3442f080 	ori	v0,v0,0xf080
ffffffff808671dc:	70451002 	mul	v0,v0,a1
ffffffff808671e0:	10000003 	b	ffffffff808671f0 <prom_init+0x27c>
ffffffff808671e4:	fc827e60 	sd	v0,32352(a0)
ffffffff808671e8:	9e0200b8 	lwu	v0,184(s0)
ffffffff808671ec:	fc827e60 	sd	v0,32352(a0)
ffffffff808671f0:	40224807 	dmfc0	v0,$9,7
ffffffff808671f4:	e85b0030 	bbit1	v0,0x1b,ffffffff808672b8 <prom_init+0x344>
ffffffff808671f8:	9f850020 	lwu	a1,32(gp)
ffffffff808671fc:	3c048017 	lui	a0,0x8017
ffffffff80867200:	3c028017 	lui	v0,0x8017
ffffffff80867204:	64872900 	daddiu	a3,a0,10496
ffffffff80867208:	64422980 	daddiu	v0,v0,10624
ffffffff8086720c:	2406004b 	li	a2,75
ffffffff80867210:	00052a38 	dsll	a1,a1,0x8
ffffffff80867214:	0047102f 	dsubu	v0,v0,a3
ffffffff80867218:	0065182d 	daddu	v1,v1,a1
ffffffff8086721c:	8c630024 	lw	v1,36(v1)
ffffffff80867220:	1066000a 	beq	v1,a2,ffffffff8086724c <prom_init+0x2d8>
ffffffff80867224:	00021000 	sll	v0,v0,0x0
ffffffff80867228:	3c118017 	lui	s1,0x8017
ffffffff8086722c:	3c148017 	lui	s4,0x8017
ffffffff80867230:	3c068017 	lui	a2,0x8017
ffffffff80867234:	3c058017 	lui	a1,0x8017
ffffffff80867238:	66312b34 	daddiu	s1,s1,11060
ffffffff8086723c:	66942b00 	daddiu	s4,s4,11008
ffffffff80867240:	64c629b8 	daddiu	a2,a2,10680
ffffffff80867244:	10000009 	b	ffffffff8086726c <prom_init+0x2f8>
ffffffff80867248:	64a52980 	daddiu	a1,a1,10624
ffffffff8086724c:	3c118017 	lui	s1,0x8017
ffffffff80867250:	3c148017 	lui	s4,0x8017
ffffffff80867254:	3c068017 	lui	a2,0x8017
ffffffff80867258:	3c058017 	lui	a1,0x8017
ffffffff8086725c:	66312b80 	daddiu	s1,s1,11136
ffffffff80867260:	66942b34 	daddiu	s4,s4,11060
ffffffff80867264:	64c62a20 	daddiu	a2,a2,10784
ffffffff80867268:	64a529b8 	daddiu	a1,a1,10680
ffffffff8086726c:	00c5302f 	dsubu	a2,a2,a1
ffffffff80867270:	00063000 	sll	a2,a2,0x0
ffffffff80867274:	0046102a 	slt	v0,v0,a2
ffffffff80867278:	1440000b 	bnez	v0,ffffffff808672a8 <prom_init+0x334>
ffffffff8086727c:	3c028017 	lui	v0,0x8017
ffffffff80867280:	3c158017 	lui	s5,0x8017
ffffffff80867284:	66b52a80 	daddiu	s5,s5,10880
ffffffff80867288:	64422b00 	daddiu	v0,v0,11008
ffffffff8086728c:	0234882f 	dsubu	s1,s1,s4
ffffffff80867290:	0055102f 	dsubu	v0,v0,s5
ffffffff80867294:	00118800 	sll	s1,s1,0x0
ffffffff80867298:	00021000 	sll	v0,v0,0x0
ffffffff8086729c:	0051102a 	slt	v0,v0,s1
ffffffff808672a0:	1040019e 	beqz	v0,ffffffff8086791c <prom_init+0x9a8>
ffffffff808672a4:	00000000 	nop
ffffffff808672a8:	3c04806f 	lui	a0,0x806f
ffffffff808672ac:	240502f9 	li	a1,761
ffffffff808672b0:	0c0637f2 	jal	ffffffff8018dfc8 <warn_slowpath_null>
ffffffff808672b4:	648471d8 	daddiu	a0,a0,29144
ffffffff808672b8:	0c041b14 	jal	ffffffff80106c50 <octeon_feature_init>
ffffffff808672bc:	00000000 	nop
ffffffff808672c0:	de437d68 	ld	v1,32104(s2)
ffffffff808672c4:	24020001 	li	v0,1
ffffffff808672c8:	94630050 	lhu	v1,80(v1)
ffffffff808672cc:	1062001a 	beq	v1,v0,ffffffff80867338 <prom_init+0x3c4>
ffffffff808672d0:	3c02808c 	lui	v0,0x808c
ffffffff808672d4:	90424381 	lbu	v0,17281(v0)
ffffffff808672d8:	c8420017 	bbit0	v0,0x2,ffffffff80867338 <prom_init+0x3c4>
ffffffff808672dc:	3c03fff0 	lui	v1,0xfff0
ffffffff808672e0:	24040020 	li	a0,32
ffffffff808672e4:	64630023 	daddiu	v1,v1,35
ffffffff808672e8:	00031afc 	dsll32	v1,v1,0xb
ffffffff808672ec:	fc601a00 	sd	zero,6656(v1)
ffffffff808672f0:	dc6500f8 	ld	a1,248(v1)
ffffffff808672f4:	fc601a10 	sd	zero,6672(v1)
ffffffff808672f8:	dc6500f8 	ld	a1,248(v1)
ffffffff808672fc:	fc601a18 	sd	zero,6680(v1)
ffffffff80867300:	dc6500f8 	ld	a1,248(v1)
ffffffff80867304:	fc601a30 	sd	zero,6704(v1)
ffffffff80867308:	dc6500f8 	ld	a1,248(v1)
ffffffff8086730c:	fc641a20 	sd	a0,6688(v1)
ffffffff80867310:	dc6500f8 	ld	a1,248(v1)
ffffffff80867314:	fc641a28 	sd	a0,6696(v1)
ffffffff80867318:	dc6400f8 	ld	a0,248(v1)
ffffffff8086731c:	fc601a38 	sd	zero,6712(v1)
ffffffff80867320:	dc6400f8 	ld	a0,248(v1)
ffffffff80867324:	fc601a40 	sd	zero,6720(v1)
ffffffff80867328:	dc6400f8 	ld	a0,248(v1)
ffffffff8086732c:	24040001 	li	a0,1
ffffffff80867330:	fc641a00 	sd	a0,6656(v1)
ffffffff80867334:	dc6200f8 	ld	v0,248(v1)
ffffffff80867338:	0c056a78 	jal	ffffffff8015a9e0 <octeon_check_cpu_bist>
ffffffff8086733c:	00000000 	nop
ffffffff80867340:	de627d70 	ld	v0,32112(s3)
ffffffff80867344:	3c04806f 	lui	a0,0x806f
ffffffff80867348:	3c03808e 	lui	v1,0x808e
ffffffff8086734c:	648471f8 	daddiu	a0,a0,29176
ffffffff80867350:	8c420140 	lw	v0,320(v0)
ffffffff80867354:	7c4200c0 	ext	v0,v0,0x3,0x1
ffffffff80867358:	0c0569f2 	jal	ffffffff8015a7c8 <octeon_write_lcd>
ffffffff8086735c:	ac627e68 	sw	v0,32360(v1)
ffffffff80867360:	0c21a9a7 	jal	ffffffff8086a69c <octeon_setup_delays>
ffffffff80867364:	00000000 	nop
ffffffff80867368:	3c03000d 	lui	v1,0xd
ffffffff8086736c:	3c0400ff 	lui	a0,0xff
ffffffff80867370:	24630001 	addiu	v1,v1,1
ffffffff80867374:	40027800 	mfc0	v0,$15
ffffffff80867378:	3484ff0f 	ori	a0,a0,0xff0f
ffffffff8086737c:	00431826 	xor	v1,v0,v1
ffffffff80867380:	00641824 	and	v1,v1,a0
ffffffff80867384:	10600011 	beqz	v1,ffffffff808673cc <prom_init+0x458>
ffffffff80867388:	3c03010d 	lui	v1,0x10d
ffffffff8086738c:	2484fff1 	addiu	a0,a0,-15
ffffffff80867390:	24630100 	addiu	v1,v1,256
ffffffff80867394:	00431026 	xor	v0,v0,v1
ffffffff80867398:	00441024 	and	v0,v0,a0
ffffffff8086739c:	1040000c 	beqz	v0,ffffffff808673d0 <prom_init+0x45c>
ffffffff808673a0:	3c02ff80 	lui	v0,0xff80
ffffffff808673a4:	3c02808c 	lui	v0,0x808c
ffffffff808673a8:	90424383 	lbu	v0,17283(v0)
ffffffff808673ac:	e846000b 	bbit1	v0,0x6,ffffffff808673dc <prom_init+0x468>
ffffffff808673b0:	3c02ff80 	lui	v0,0xff80
ffffffff808673b4:	24030001 	li	v1,1
ffffffff808673b8:	64420107 	daddiu	v0,v0,263
ffffffff808673bc:	0002123c 	dsll32	v0,v0,0x8
ffffffff808673c0:	fc430738 	sd	v1,1848(v0)
ffffffff808673c4:	10000006 	b	ffffffff808673e0 <prom_init+0x46c>
ffffffff808673c8:	de437d68 	ld	v1,32104(s2)
ffffffff808673cc:	3c02ff80 	lui	v0,0xff80
ffffffff808673d0:	64420107 	daddiu	v0,v0,263
ffffffff808673d4:	0002123c 	dsll32	v0,v0,0x8
ffffffff808673d8:	fc400738 	sd	zero,1848(v0)
ffffffff808673dc:	de437d68 	ld	v1,32104(s2)
ffffffff808673e0:	24020001 	li	v0,1
ffffffff808673e4:	94630050 	lhu	v1,80(v1)
ffffffff808673e8:	14620003 	bne	v1,v0,ffffffff808673f8 <prom_init+0x484>
ffffffff808673ec:	3c028082 	lui	v0,0x8082
ffffffff808673f0:	3c030400 	lui	v1,0x400
ffffffff808673f4:	fc436218 	sd	v1,25112(v0)
ffffffff808673f8:	de020000 	ld	v0,0(s0)
ffffffff808673fc:	10400003 	beqz	v0,ffffffff8086740c <prom_init+0x498>
ffffffff80867400:	3c14806f 	lui	s4,0x806f
ffffffff80867404:	3c038082 	lui	v1,0x8082
ffffffff80867408:	fc626218 	sd	v0,25112(v1)
ffffffff8086740c:	3c11808a 	lui	s1,0x808a
ffffffff80867410:	66857208 	daddiu	a1,s4,29192
ffffffff80867414:	0c0e3f28 	jal	ffffffff8038fca0 <strstr>
ffffffff80867418:	6624a008 	daddiu	a0,s1,-24568
ffffffff8086741c:	10400012 	beqz	v0,ffffffff80867468 <prom_init+0x4f4>
ffffffff80867420:	ffb40018 	sd	s4,24(sp)
ffffffff80867424:	03a02825 	move	a1,sp
ffffffff80867428:	0c0e1362 	jal	ffffffff80384d88 <memparse>
ffffffff8086742c:	64440004 	daddiu	a0,v0,4
ffffffff80867430:	00021538 	dsll	v0,v0,0x14
ffffffff80867434:	14400003 	bnez	v0,ffffffff80867444 <prom_init+0x4d0>
ffffffff80867438:	3c038082 	lui	v1,0x8082
ffffffff8086743c:	24020001 	li	v0,1
ffffffff80867440:	000214bc 	dsll32	v0,v0,0x12
ffffffff80867444:	dfa40000 	ld	a0,0(sp)
ffffffff80867448:	fc626218 	sd	v0,25112(v1)
ffffffff8086744c:	24020040 	li	v0,64
ffffffff80867450:	80830000 	lb	v1,0(a0)
ffffffff80867454:	14620005 	bne	v1,v0,ffffffff8086746c <prom_init+0x4f8>
ffffffff80867458:	de627d70 	ld	v0,32112(s3)
ffffffff8086745c:	03a02825 	move	a1,sp
ffffffff80867460:	0c0e1362 	jal	ffffffff80384d88 <memparse>
ffffffff80867464:	64840001 	daddiu	a0,a0,1
ffffffff80867468:	de627d70 	ld	v0,32112(s3)
ffffffff8086746c:	3c03806f 	lui	v1,0x806f
ffffffff80867470:	a220a008 	sb	zero,-24568(s1)
ffffffff80867474:	3c14808a 	lui	s4,0x808a
ffffffff80867478:	0000a825 	move	s5,zero
ffffffff8086747c:	66949130 	daddiu	s4,s4,-28368
ffffffff80867480:	8c42003c 	lw	v0,60(v0)
ffffffff80867484:	ffa30020 	sd	v1,32(sp)
ffffffff80867488:	ffa20010 	sd	v0,16(sp)
ffffffff8086748c:	3c028082 	lui	v0,0x8082
ffffffff80867490:	ffa20028 	sd	v0,40(sp)
ffffffff80867494:	24020001 	li	v0,1
ffffffff80867498:	000214bc 	dsll32	v0,v0,0x12
ffffffff8086749c:	ffa20030 	sd	v0,48(sp)
ffffffff808674a0:	dfa20010 	ld	v0,16(sp)
ffffffff808674a4:	02a2102a 	slt	v0,s5,v0
ffffffff808674a8:	10400081 	beqz	v0,ffffffff808676b0 <prom_init+0x73c>
ffffffff808674ac:	de767d70 	ld	s6,32112(s3)
ffffffff808674b0:	66a20010 	daddiu	v0,s5,16
ffffffff808674b4:	000210b8 	dsll	v0,v0,0x2
ffffffff808674b8:	2406000a 	li	a2,10
ffffffff808674bc:	7ec2b40a 	lwux	s6,v0(s6)
ffffffff808674c0:	dfa20020 	ld	v0,32(sp)
ffffffff808674c4:	64457210 	daddiu	a1,v0,29200
ffffffff808674c8:	2402ffff 	li	v0,-1
ffffffff808674cc:	000217fc 	dsll32	v0,v0,0x1f
ffffffff808674d0:	02c2b02d 	daddu	s6,s6,v0
ffffffff808674d4:	0c0e3d96 	jal	ffffffff8038f658 <strncmp>
ffffffff808674d8:	02c02025 	move	a0,s6
ffffffff808674dc:	14400032 	bnez	v0,ffffffff808675a8 <prom_init+0x634>
ffffffff808674e0:	66d7000a 	daddiu	s7,s6,10
ffffffff808674e4:	0000b025 	move	s6,zero
ffffffff808674e8:	82e20000 	lb	v0,0(s7)
ffffffff808674ec:	14400017 	bnez	v0,ffffffff8086754c <prom_init+0x5d8>
ffffffff808674f0:	24020004 	li	v0,4
ffffffff808674f4:	3c16806f 	lui	s6,0x806f
ffffffff808674f8:	02803025 	move	a2,s4
ffffffff808674fc:	66c47220 	daddiu	a0,s6,29216
ffffffff80867500:	0c189d79 	jal	ffffffff806275e4 <printk>
ffffffff80867504:	00002825 	move	a1,zero
ffffffff80867508:	3c06808a 	lui	a2,0x808a
ffffffff8086750c:	66c47220 	daddiu	a0,s6,29216
ffffffff80867510:	64c691b0 	daddiu	a2,a2,-28240
ffffffff80867514:	0c189d79 	jal	ffffffff806275e4 <printk>
ffffffff80867518:	24050001 	li	a1,1
ffffffff8086751c:	3c06808a 	lui	a2,0x808a
ffffffff80867520:	66c47220 	daddiu	a0,s6,29216
ffffffff80867524:	64c69230 	daddiu	a2,a2,-28112
ffffffff80867528:	0c189d79 	jal	ffffffff806275e4 <printk>
ffffffff8086752c:	24050002 	li	a1,2
ffffffff80867530:	3c06808a 	lui	a2,0x808a
ffffffff80867534:	64c692b0 	daddiu	a2,a2,-27984
ffffffff80867538:	24050003 	li	a1,3
ffffffff8086753c:	0c189d79 	jal	ffffffff806275e4 <printk>
ffffffff80867540:	66c47220 	daddiu	a0,s6,29216
ffffffff80867544:	1000ffd6 	b	ffffffff808674a0 <prom_init+0x52c>
ffffffff80867548:	26b50001 	addiu	s5,s5,1
ffffffff8086754c:	12c2ffe9 	beq	s6,v0,ffffffff808674f4 <prom_init+0x580>
ffffffff80867550:	2405002c 	li	a1,44
ffffffff80867554:	0c0e3db4 	jal	ffffffff8038f6d0 <strchr>
ffffffff80867558:	02e02025 	move	a0,s7
ffffffff8086755c:	0057302f 	dsubu	a2,v0,s7
ffffffff80867560:	0040f025 	move	s8,v0
ffffffff80867564:	14400007 	bnez	v0,ffffffff80867584 <prom_init+0x610>
ffffffff80867568:	00063000 	sll	a2,a2,0x0
ffffffff8086756c:	0c0e3e2e 	jal	ffffffff8038f8b8 <strlen>
ffffffff80867570:	02e02025 	move	a0,s7
ffffffff80867574:	24030080 	li	v1,128
ffffffff80867578:	2c460080 	sltiu	a2,v0,128
ffffffff8086757c:	0046180a 	movz	v1,v0,a2
ffffffff80867580:	00033000 	sll	a2,v1,0x0
ffffffff80867584:	001621f8 	dsll	a0,s6,0x7
ffffffff80867588:	02e02825 	move	a1,s7
ffffffff8086758c:	0284202d 	daddu	a0,s4,a0
ffffffff80867590:	0c0e3d50 	jal	ffffffff8038f540 <strncpy>
ffffffff80867594:	66d60001 	daddiu	s6,s6,1
ffffffff80867598:	13c0ffd6 	beqz	s8,ffffffff808674f4 <prom_init+0x580>
ffffffff8086759c:	00000000 	nop
ffffffff808675a0:	1000ffd1 	b	ffffffff808674e8 <prom_init+0x574>
ffffffff808675a4:	67d70001 	daddiu	s7,s8,1
ffffffff808675a8:	3c05806f 	lui	a1,0x806f
ffffffff808675ac:	24060004 	li	a2,4
ffffffff808675b0:	64a57240 	daddiu	a1,a1,29248
ffffffff808675b4:	0c0e3d96 	jal	ffffffff8038f658 <strncmp>
ffffffff808675b8:	02c02025 	move	a0,s6
ffffffff808675bc:	14400012 	bnez	v0,ffffffff80867608 <prom_init+0x694>
ffffffff808675c0:	dfa20018 	ld	v0,24(sp)
ffffffff808675c4:	03a02825 	move	a1,sp
ffffffff808675c8:	0c0e1362 	jal	ffffffff80384d88 <memparse>
ffffffff808675cc:	66c40004 	daddiu	a0,s6,4
ffffffff808675d0:	dfa60030 	ld	a2,48(sp)
ffffffff808675d4:	dfa40000 	ld	a0,0(sp)
ffffffff808675d8:	24030040 	li	v1,64
ffffffff808675dc:	00021538 	dsll	v0,v0,0x14
ffffffff808675e0:	00c2100a 	movz	v0,a2,v0
ffffffff808675e4:	80850000 	lb	a1,0(a0)
ffffffff808675e8:	dfa60028 	ld	a2,40(sp)
ffffffff808675ec:	14a3002e 	bne	a1,v1,ffffffff808676a8 <prom_init+0x734>
ffffffff808675f0:	fcc26218 	sd	v0,25112(a2)
ffffffff808675f4:	03a02825 	move	a1,sp
ffffffff808675f8:	0c0e1362 	jal	ffffffff80384d88 <memparse>
ffffffff808675fc:	64840001 	daddiu	a0,a0,1
ffffffff80867600:	1000ffa7 	b	ffffffff808674a0 <prom_init+0x52c>
ffffffff80867604:	26b50001 	addiu	s5,s5,1
ffffffff80867608:	24060004 	li	a2,4
ffffffff8086760c:	02c02025 	move	a0,s6
ffffffff80867610:	0c0e3d96 	jal	ffffffff8038f658 <strncmp>
ffffffff80867614:	64457208 	daddiu	a1,v0,29192
ffffffff80867618:	1040ffeb 	beqz	v0,ffffffff808675c8 <prom_init+0x654>
ffffffff8086761c:	03a02825 	move	a1,sp
ffffffff80867620:	3c05806f 	lui	a1,0x806f
ffffffff80867624:	24060008 	li	a2,8
ffffffff80867628:	64a57248 	daddiu	a1,a1,29256
ffffffff8086762c:	0c0e3d96 	jal	ffffffff8038f658 <strncmp>
ffffffff80867630:	02c02025 	move	a0,s6
ffffffff80867634:	14400009 	bnez	v0,ffffffff8086765c <prom_init+0x6e8>
ffffffff80867638:	3c05806f 	lui	a1,0x806f
ffffffff8086763c:	3c17808a 	lui	s7,0x808a
ffffffff80867640:	24060040 	li	a2,64
ffffffff80867644:	66e49330 	daddiu	a0,s7,-27856
ffffffff80867648:	66c50008 	daddiu	a1,s6,8
ffffffff8086764c:	0c0e3d50 	jal	ffffffff8038f540 <strncpy>
ffffffff80867650:	66f79330 	daddiu	s7,s7,-27856
ffffffff80867654:	10000004 	b	ffffffff80867668 <prom_init+0x6f4>
ffffffff80867658:	a2e0003f 	sb	zero,63(s7)
ffffffff8086765c:	02c02025 	move	a0,s6
ffffffff80867660:	0c0e3d84 	jal	ffffffff8038f610 <strcmp>
ffffffff80867664:	64a57258 	daddiu	a1,a1,29272
ffffffff80867668:	0c0e3e2e 	jal	ffffffff8038f8b8 <strlen>
ffffffff8086766c:	6624a008 	daddiu	a0,s1,-24568
ffffffff80867670:	02c02025 	move	a0,s6
ffffffff80867674:	0c0e3e2e 	jal	ffffffff8038f8b8 <strlen>
ffffffff80867678:	0040b825 	move	s7,v0
ffffffff8086767c:	02e2102d 	daddu	v0,s7,v0
ffffffff80867680:	64420001 	daddiu	v0,v0,1
ffffffff80867684:	2c420fff 	sltiu	v0,v0,4095
ffffffff80867688:	10400007 	beqz	v0,ffffffff808676a8 <prom_init+0x734>
ffffffff8086768c:	3c058072 	lui	a1,0x8072
ffffffff80867690:	6624a008 	daddiu	a0,s1,-24568
ffffffff80867694:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff80867698:	64a50ba8 	daddiu	a1,a1,2984
ffffffff8086769c:	02c02825 	move	a1,s6
ffffffff808676a0:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff808676a4:	6624a008 	daddiu	a0,s1,-24568
ffffffff808676a8:	1000ff7d 	b	ffffffff808674a0 <prom_init+0x52c>
ffffffff808676ac:	26b50001 	addiu	s5,s5,1
ffffffff808676b0:	3c13806f 	lui	s3,0x806f
ffffffff808676b4:	6624a008 	daddiu	a0,s1,-24568
ffffffff808676b8:	0c0e3f28 	jal	ffffffff8038fca0 <strstr>
ffffffff808676bc:	66657268 	daddiu	a1,s3,29288
ffffffff808676c0:	10400005 	beqz	v0,ffffffff808676d8 <prom_init+0x764>
ffffffff808676c4:	6624a008 	daddiu	a0,s1,-24568
ffffffff808676c8:	0c0e3f28 	jal	ffffffff8038fca0 <strstr>
ffffffff808676cc:	66657268 	daddiu	a1,s3,29288
ffffffff808676d0:	0c058890 	jal	ffffffff80162240 <octeon_pci_console_init>
ffffffff808676d4:	64440008 	daddiu	a0,v0,8
ffffffff808676d8:	3c05806f 	lui	a1,0x806f
ffffffff808676dc:	6624a008 	daddiu	a0,s1,-24568
ffffffff808676e0:	0c0e3f28 	jal	ffffffff8038fca0 <strstr>
ffffffff808676e4:	64a57278 	daddiu	a1,a1,29304
ffffffff808676e8:	14400004 	bnez	v0,ffffffff808676fc <prom_init+0x788>
ffffffff808676ec:	6624a008 	daddiu	a0,s1,-24568
ffffffff808676f0:	3c05806f 	lui	a1,0x806f
ffffffff808676f4:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff808676f8:	64a57288 	daddiu	a1,a1,29320
ffffffff808676fc:	92020110 	lbu	v0,272(s0)
ffffffff80867700:	2442fffe 	addiu	v0,v0,-2
ffffffff80867704:	304300ff 	andi	v1,v0,0xff
ffffffff80867708:	2c630005 	sltiu	v1,v1,5
ffffffff8086770c:	1060001d 	beqz	v1,ffffffff80867784 <prom_init+0x810>
ffffffff80867710:	3c038064 	lui	v1,0x8064
ffffffff80867714:	704238f2 	cins	v0,v0,0x3,0x7
ffffffff80867718:	6463c080 	daddiu	v1,v1,-16256
ffffffff8086771c:	7c62120a 	ldx	v0,v0(v1)
ffffffff80867720:	00400008 	jr	v0
ffffffff80867724:	6624a008 	daddiu	a0,s1,-24568
ffffffff80867728:	3c05806f 	lui	a1,0x806f
ffffffff8086772c:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff80867730:	64a572a0 	daddiu	a1,a1,29344
ffffffff80867734:	10000014 	b	ffffffff80867788 <prom_init+0x814>
ffffffff80867738:	92020111 	lbu	v0,273(s0)
ffffffff8086773c:	3c05806f 	lui	a1,0x806f
ffffffff80867740:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff80867744:	64a572b0 	daddiu	a1,a1,29360
ffffffff80867748:	1000000f 	b	ffffffff80867788 <prom_init+0x814>
ffffffff8086774c:	92020111 	lbu	v0,273(s0)
ffffffff80867750:	3c05806f 	lui	a1,0x806f
ffffffff80867754:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff80867758:	64a572c0 	daddiu	a1,a1,29376
ffffffff8086775c:	1000000a 	b	ffffffff80867788 <prom_init+0x814>
ffffffff80867760:	92020111 	lbu	v0,273(s0)
ffffffff80867764:	3c05806f 	lui	a1,0x806f
ffffffff80867768:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff8086776c:	64a572d0 	daddiu	a1,a1,29392
ffffffff80867770:	10000005 	b	ffffffff80867788 <prom_init+0x814>
ffffffff80867774:	92020111 	lbu	v0,273(s0)
ffffffff80867778:	3c05806f 	lui	a1,0x806f
ffffffff8086777c:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff80867780:	64a572e0 	daddiu	a1,a1,29408
ffffffff80867784:	92020111 	lbu	v0,273(s0)
ffffffff80867788:	24030001 	li	v1,1
ffffffff8086778c:	10430012 	beq	v0,v1,ffffffff808677d8 <prom_init+0x864>
ffffffff80867790:	6624a008 	daddiu	a0,s1,-24568
ffffffff80867794:	1040000b 	beqz	v0,ffffffff808677c4 <prom_init+0x850>
ffffffff80867798:	00000000 	nop
ffffffff8086779c:	24030002 	li	v1,2
ffffffff808677a0:	10430012 	beq	v0,v1,ffffffff808677ec <prom_init+0x878>
ffffffff808677a4:	24030003 	li	v1,3
ffffffff808677a8:	14430014 	bne	v0,v1,ffffffff808677fc <prom_init+0x888>
ffffffff808677ac:	00000000 	nop
ffffffff808677b0:	3c05806f 	lui	a1,0x806f
ffffffff808677b4:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff808677b8:	64a57320 	daddiu	a1,a1,29472
ffffffff808677bc:	10000010 	b	ffffffff80867800 <prom_init+0x88c>
ffffffff808677c0:	de437d68 	ld	v1,32104(s2)
ffffffff808677c4:	3c05806f 	lui	a1,0x806f
ffffffff808677c8:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff808677cc:	64a572f0 	daddiu	a1,a1,29424
ffffffff808677d0:	1000000b 	b	ffffffff80867800 <prom_init+0x88c>
ffffffff808677d4:	de437d68 	ld	v1,32104(s2)
ffffffff808677d8:	3c05806f 	lui	a1,0x806f
ffffffff808677dc:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff808677e0:	64a57300 	daddiu	a1,a1,29440
ffffffff808677e4:	10000006 	b	ffffffff80867800 <prom_init+0x88c>
ffffffff808677e8:	de437d68 	ld	v1,32104(s2)
ffffffff808677ec:	3c05806f 	lui	a1,0x806f
ffffffff808677f0:	6624a008 	daddiu	a0,s1,-24568
ffffffff808677f4:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff808677f8:	64a57310 	daddiu	a1,a1,29456
ffffffff808677fc:	de437d68 	ld	v1,32104(s2)
ffffffff80867800:	24020001 	li	v0,1
ffffffff80867804:	94630050 	lhu	v1,80(v1)
ffffffff80867808:	14620004 	bne	v1,v0,ffffffff8086781c <prom_init+0x8a8>
ffffffff8086780c:	3c05806f 	lui	a1,0x806f
ffffffff80867810:	6624a008 	daddiu	a0,s1,-24568
ffffffff80867814:	0c0e3d5c 	jal	ffffffff8038f570 <strcat>
ffffffff80867818:	64a57330 	daddiu	a1,a1,29488
ffffffff8086781c:	0c0415f4 	jal	ffffffff801057d0 <cvmx_sysinfo_get>
ffffffff80867820:	00000000 	nop
ffffffff80867824:	8c4300b8 	lw	v1,184(v0)
ffffffff80867828:	3c02808f 	lui	v0,0x808f
ffffffff8086782c:	0c057f3e 	jal	ffffffff8015fcf8 <octeon_init_cvmcount>
ffffffff80867830:	ac43b3a0 	sw	v1,-19552(v0)
ffffffff80867834:	3c028016 	lui	v0,0x8016
ffffffff80867838:	3c03808f 	lui	v1,0x808f
ffffffff8086783c:	6442ac38 	daddiu	v0,v0,-21448
ffffffff80867840:	fc62afc0 	sd	v0,-20544(v1)
ffffffff80867844:	3c028016 	lui	v0,0x8016
ffffffff80867848:	3c03808f 	lui	v1,0x808f
ffffffff8086784c:	6442a858 	daddiu	v0,v0,-22440
ffffffff80867850:	0c056bac 	jal	ffffffff8015aeb0 <octeon_user_io_init>
ffffffff80867854:	fc62afb8 	sd	v0,-20552(v1)
ffffffff80867858:	0c21abe0 	jal	ffffffff8086af80 <octeon_setup_smp>
ffffffff8086785c:	00000000 	nop
ffffffff80867860:	3c02808c 	lui	v0,0x808c
ffffffff80867864:	90434380 	lbu	v1,17280(v0)
ffffffff80867868:	c8660009 	bbit0	v1,0x6,ffffffff80867890 <prom_init+0x91c>
ffffffff8086786c:	3c058094 	lui	a1,0x8094
ffffffff80867870:	64424380 	daddiu	v0,v0,17280
ffffffff80867874:	24040003 	li	a0,3
ffffffff80867878:	90420002 	lbu	v0,2(v0)
ffffffff8086787c:	24030004 	li	v1,4
ffffffff80867880:	7c420080 	ext	v0,v0,0x2,0x1
ffffffff80867884:	0082180b 	movn	v1,a0,v0
ffffffff80867888:	10000015 	b	ffffffff808678e0 <prom_init+0x96c>
ffffffff8086788c:	aca3f8c0 	sw	v1,-1856(a1)
ffffffff80867890:	3c03010d 	lui	v1,0x10d
ffffffff80867894:	3c0400ff 	lui	a0,0xff
ffffffff80867898:	24630100 	addiu	v1,v1,256
ffffffff8086789c:	3484ff00 	ori	a0,a0,0xff00
ffffffff808678a0:	40027800 	mfc0	v0,$15
ffffffff808678a4:	00431826 	xor	v1,v0,v1
ffffffff808678a8:	00641824 	and	v1,v1,a0
ffffffff808678ac:	1060000a 	beqz	v1,ffffffff808678d8 <prom_init+0x964>
ffffffff808678b0:	3c03000d 	lui	v1,0xd
ffffffff808678b4:	2484000f 	addiu	a0,a0,15
ffffffff808678b8:	24630001 	addiu	v1,v1,1
ffffffff808678bc:	00431026 	xor	v0,v0,v1
ffffffff808678c0:	00441024 	and	v0,v0,a0
ffffffff808678c4:	10400005 	beqz	v0,ffffffff808678dc <prom_init+0x968>
ffffffff808678c8:	24020001 	li	v0,1
ffffffff808678cc:	24020002 	li	v0,2
ffffffff808678d0:	10000003 	b	ffffffff808678e0 <prom_init+0x96c>
ffffffff808678d4:	aca2f8c0 	sw	v0,-1856(a1)
ffffffff808678d8:	24020001 	li	v0,1
ffffffff808678dc:	aca2f8c0 	sw	v0,-1856(a1)
ffffffff808678e0:	3c04806f 	lui	a0,0x806f
ffffffff808678e4:	0c189d79 	jal	ffffffff806275e4 <printk>
ffffffff808678e8:	64847360 	daddiu	a0,a0,29536
ffffffff808678ec:	dfbf0088 	ld	ra,136(sp)
ffffffff808678f0:	dfbe0080 	ld	s8,128(sp)
ffffffff808678f4:	dfb70078 	ld	s7,120(sp)
ffffffff808678f8:	dfb60070 	ld	s6,112(sp)
ffffffff808678fc:	dfb50068 	ld	s5,104(sp)
ffffffff80867900:	dfb40060 	ld	s4,96(sp)
ffffffff80867904:	dfb30058 	ld	s3,88(sp)
ffffffff80867908:	dfb20050 	ld	s2,80(sp)
ffffffff8086790c:	dfb10048 	ld	s1,72(sp)
ffffffff80867910:	dfb00040 	ld	s0,64(sp)
ffffffff80867914:	03e00008 	jr	ra
ffffffff80867918:	67bd0090 	daddiu	sp,sp,144
ffffffff8086791c:	0c0588d0 	jal	ffffffff80162340 <memcpy>
ffffffff80867920:	64842900 	daddiu	a0,a0,10496
ffffffff80867924:	02203025 	move	a2,s1
ffffffff80867928:	02802825 	move	a1,s4
ffffffff8086792c:	0c0588d0 	jal	ffffffff80162340 <memcpy>
ffffffff80867930:	02a02025 	move	a0,s5
ffffffff80867934:	1000fe60 	b	ffffffff808672b8 <prom_init+0x344>
ffffffff80867938:	00000000 	nop

Disassembly of section .exit.text:
~~~

## Disassembly: device_tree_init

~~~asm

artifacts/pa220/original-kernel/pa220-original-vmlinux:     file format elf64-tradbigmips


Disassembly of section .text:

Disassembly of section .init.text:

ffffffff80867f40 <device_tree_init>:
ffffffff80867f40:	3c02808e 	lui	v0,0x808e
ffffffff80867f44:	67bdffe0 	daddiu	sp,sp,-32
ffffffff80867f48:	dc437d68 	ld	v1,32104(v0)
ffffffff80867f4c:	ffbf0018 	sd	ra,24(sp)
ffffffff80867f50:	ffb20010 	sd	s2,16(sp)
ffffffff80867f54:	ffb10008 	sd	s1,8(sp)
ffffffff80867f58:	ffb00000 	sd	s0,0(sp)
ffffffff80867f5c:	8c620004 	lw	v0,4(v1)
ffffffff80867f60:	2c420003 	sltiu	v0,v0,3
ffffffff80867f64:	14400014 	bnez	v0,ffffffff80867fb8 <device_tree_init+0x78>
ffffffff80867f68:	00000000 	nop
ffffffff80867f6c:	dc7100b0 	ld	s1,176(v1)
ffffffff80867f70:	12200012 	beqz	s1,ffffffff80867fbc <device_tree_init+0x7c>
ffffffff80867f74:	3c03010d 	lui	v1,0x10d
ffffffff80867f78:	2402ffff 	li	v0,-1
ffffffff80867f7c:	3c04806f 	lui	a0,0x806f
ffffffff80867f80:	000217fc 	dsll32	v0,v0,0x1f
ffffffff80867f84:	64847588 	daddiu	a0,a0,30088
ffffffff80867f88:	0222882d 	daddu	s1,s1,v0
ffffffff80867f8c:	0c189d79 	jal	ffffffff806275e4 <printk>
ffffffff80867f90:	02202825 	move	a1,s1
ffffffff80867f94:	0c0e1474 	jal	ffffffff803851d0 <fdt_check_header>
ffffffff80867f98:	02202025 	move	a0,s1
ffffffff80867f9c:	10400003 	beqz	v0,ffffffff80867fac <device_tree_init+0x6c>
ffffffff80867fa0:	3c04806f 	lui	a0,0x806f
ffffffff80867fa4:	0c189cf7 	jal	ffffffff806273dc <panic>
ffffffff80867fa8:	648475b0 	daddiu	a0,a0,30128
ffffffff80867fac:	8e300004 	lw	s0,4(s1)
ffffffff80867fb0:	10000018 	b	ffffffff80868014 <device_tree_init+0xd4>
ffffffff80867fb4:	00009025 	move	s2,zero
ffffffff80867fb8:	3c03010d 	lui	v1,0x10d
ffffffff80867fbc:	34639108 	ori	v1,v1,0x9108
ffffffff80867fc0:	40027800 	mfc0	v0,$15
ffffffff80867fc4:	00431026 	xor	v0,v0,v1
ffffffff80867fc8:	3c0300ff 	lui	v1,0xff
ffffffff80867fcc:	3463ff00 	ori	v1,v1,0xff00
ffffffff80867fd0:	00431024 	and	v0,v0,v1
ffffffff80867fd4:	14400008 	bnez	v0,ffffffff80867ff8 <device_tree_init+0xb8>
ffffffff80867fd8:	3c11808a 	lui	s1,0x808a
ffffffff80867fdc:	3c10808b 	lui	s0,0x808b
ffffffff80867fe0:	66317540 	daddiu	s1,s1,30016
ffffffff80867fe4:	6610a4ed 	daddiu	s0,s0,-23315
ffffffff80867fe8:	0211802f 	dsubu	s0,s0,s1
ffffffff80867fec:	24120001 	li	s2,1
ffffffff80867ff0:	10000008 	b	ffffffff80868014 <device_tree_init+0xd4>
ffffffff80867ff4:	00108000 	sll	s0,s0,0x0
ffffffff80867ff8:	3c11808a 	lui	s1,0x808a
ffffffff80867ffc:	3c10808a 	lui	s0,0x808a
ffffffff80868000:	663147e0 	daddiu	s1,s1,18400
ffffffff80868004:	66107531 	daddiu	s0,s0,30001
ffffffff80868008:	0211802f 	dsubu	s0,s0,s1
ffffffff8086800c:	24120001 	li	s2,1
ffffffff80868010:	00108000 	sll	s0,s0,0x0
ffffffff80868014:	24050008 	li	a1,8
ffffffff80868018:	0c21b371 	jal	ffffffff8086cdc4 <early_init_dt_alloc_memory_arch>
ffffffff8086801c:	02002025 	move	a0,s0
ffffffff80868020:	3c038094 	lui	v1,0x8094
ffffffff80868024:	14400004 	bnez	v0,ffffffff80868038 <device_tree_init+0xf8>
ffffffff80868028:	fc62ed70 	sd	v0,-4752(v1)
ffffffff8086802c:	3c04806f 	lui	a0,0x806f
ffffffff80868030:	0c189cf7 	jal	ffffffff806273dc <panic>
ffffffff80868034:	648475d8 	daddiu	a0,a0,30168
ffffffff80868038:	02003025 	move	a2,s0
ffffffff8086803c:	02202825 	move	a1,s1
ffffffff80868040:	0c0588d0 	jal	ffffffff80162340 <memcpy>
ffffffff80868044:	00402025 	move	a0,v0
ffffffff80868048:	12400007 	beqz	s2,ffffffff80868068 <device_tree_init+0x128>
ffffffff8086804c:	dfbf0018 	ld	ra,24(sp)
ffffffff80868050:	0c21a2d6 	jal	ffffffff80868b58 <octeon_prune_device_tree>
ffffffff80868054:	00000000 	nop
ffffffff80868058:	3c04806f 	lui	a0,0x806f
ffffffff8086805c:	0c189d79 	jal	ffffffff806275e4 <printk>
ffffffff80868060:	64847600 	daddiu	a0,a0,30208
ffffffff80868064:	dfbf0018 	ld	ra,24(sp)
ffffffff80868068:	dfb20010 	ld	s2,16(sp)
ffffffff8086806c:	dfb10008 	ld	s1,8(sp)
ffffffff80868070:	dfb00000 	ld	s0,0(sp)
ffffffff80868074:	08220ab3 	j	ffffffff80882acc <unflatten_device_tree>
ffffffff80868078:	67bd0020 	daddiu	sp,sp,32

Disassembly of section .exit.text:
~~~

## Disassembly: plat_mem_setup

~~~asm

artifacts/pa220/original-kernel/pa220-original-vmlinux:     file format elf64-tradbigmips


Disassembly of section .text:

Disassembly of section .init.text:

ffffffff8086793c <plat_mem_setup>:
ffffffff8086793c:	67bdffa0 	daddiu	sp,sp,-96
ffffffff80867940:	3c040040 	lui	a0,0x40
ffffffff80867944:	ffbf0058 	sd	ra,88(sp)
ffffffff80867948:	ffb10020 	sd	s1,32(sp)
ffffffff8086794c:	ffb70050 	sd	s7,80(sp)
ffffffff80867950:	ffb60048 	sd	s6,72(sp)
ffffffff80867954:	ffb50040 	sd	s5,64(sp)
ffffffff80867958:	ffb40038 	sd	s4,56(sp)
ffffffff8086795c:	ffb30030 	sd	s3,48(sp)
ffffffff80867960:	ffb20028 	sd	s2,40(sp)
ffffffff80867964:	0c0404b0 	jal	ffffffff801012c0 <cvmx_bootmem_available_mem>
ffffffff80867968:	ffb00018 	sd	s0,24(sp)
ffffffff8086796c:	0c0415f4 	jal	ffffffff801057d0 <cvmx_sysinfo_get>
ffffffff80867970:	00408825 	move	s1,v0
ffffffff80867974:	2403ffff 	li	v1,-1
ffffffff80867978:	00002025 	move	a0,zero
ffffffff8086797c:	64450028 	daddiu	a1,v0,40
ffffffff80867980:	24060008 	li	a2,8
ffffffff80867984:	dca70000 	ld	a3,0(a1)
ffffffff80867988:	0087180b 	movn	v1,a0,a3
ffffffff8086798c:	24840001 	addiu	a0,a0,1
ffffffff80867990:	1486fffc 	bne	a0,a2,ffffffff80867984 <plat_mem_setup+0x48>
ffffffff80867994:	64a50008 	daddiu	a1,a1,8
ffffffff80867998:	2404ffff 	li	a0,-1
ffffffff8086799c:	1064000d 	beq	v1,a0,ffffffff808679d4 <plat_mem_setup+0x98>
ffffffff808679a0:	000320f8 	dsll	a0,v1,0x3
ffffffff808679a4:	00031980 	sll	v1,v1,0x6
ffffffff808679a8:	0044102d 	daddu	v0,v0,a0
ffffffff808679ac:	dc420028 	ld	v0,40(v0)
ffffffff808679b0:	70421024 	dclz	v0,v0
ffffffff808679b4:	00621823 	subu	v1,v1,v0
ffffffff808679b8:	2463003f 	addiu	v1,v1,63
ffffffff808679bc:	28630080 	slti	v1,v1,128
ffffffff808679c0:	14600005 	bnez	v1,ffffffff808679d8 <plat_mem_setup+0x9c>
ffffffff808679c4:	3c12808a 	lui	s2,0x808a
ffffffff808679c8:	3c04806f 	lui	a0,0x806f
ffffffff808679cc:	0c189cf7 	jal	ffffffff806273dc <panic>
ffffffff808679d0:	64847380 	daddiu	a0,a0,29568
ffffffff808679d4:	3c12808a 	lui	s2,0x808a
ffffffff808679d8:	82429130 	lb	v0,-28368(s2)
ffffffff808679dc:	10400047 	beqz	v0,ffffffff80867afc <plat_mem_setup+0x1c0>
ffffffff808679e0:	3c148082 	lui	s4,0x8082
ffffffff808679e4:	3c048010 	lui	a0,0x8010
ffffffff808679e8:	2410f000 	li	s0,-4096
ffffffff808679ec:	64840000 	daddiu	a0,a0,0
ffffffff808679f0:	3c14806f 	lui	s4,0x806f
ffffffff808679f4:	0c05f988 	jal	ffffffff8017e620 <__phys_addr>
ffffffff808679f8:	66529130 	daddiu	s2,s2,-28368
ffffffff808679fc:	3c048095 	lui	a0,0x8095
ffffffff80867a00:	64842640 	daddiu	a0,a0,9792
ffffffff80867a04:	0050a824 	and	s5,v0,s0
ffffffff80867a08:	24160005 	li	s6,5
ffffffff80867a0c:	0c05f988 	jal	ffffffff8017e620 <__phys_addr>
ffffffff80867a10:	00008825 	move	s1,zero
ffffffff80867a14:	669473f0 	daddiu	s4,s4,29680
ffffffff80867a18:	64420fff 	daddiu	v0,v0,4095
ffffffff80867a1c:	00508024 	and	s0,v0,s0
ffffffff80867a20:	26d6ffff 	addiu	s6,s6,-1
ffffffff80867a24:	12c00076 	beqz	s6,ffffffff80867c00 <plat_mem_setup+0x2c4>
ffffffff80867a28:	3c02808e 	lui	v0,0x808e
ffffffff80867a2c:	82420000 	lb	v0,0(s2)
ffffffff80867a30:	10400073 	beqz	v0,ffffffff80867c00 <plat_mem_setup+0x2c4>
ffffffff80867a34:	3c02808e 	lui	v0,0x808e
ffffffff80867a38:	0c040564 	jal	ffffffff80101590 <cvmx_bootmem_find_named_block>
ffffffff80867a3c:	02402025 	move	a0,s2
ffffffff80867a40:	14400007 	bnez	v0,ffffffff80867a60 <plat_mem_setup+0x124>
ffffffff80867a44:	00409825 	move	s3,v0
ffffffff80867a48:	3c04806f 	lui	a0,0x806f
ffffffff80867a4c:	02402825 	move	a1,s2
ffffffff80867a50:	0c189d79 	jal	ffffffff806275e4 <printk>
ffffffff80867a54:	648473c0 	daddiu	a0,a0,29632
ffffffff80867a58:	1000007b 	b	ffffffff80867c48 <plat_mem_setup+0x30c>
ffffffff80867a5c:	dfbf0058 	ld	ra,88(sp)
ffffffff80867a60:	dc470000 	ld	a3,0(v0)
ffffffff80867a64:	02402825 	move	a1,s2
ffffffff80867a68:	dc460008 	ld	a2,8(v0)
ffffffff80867a6c:	0c189d79 	jal	ffffffff806275e4 <printk>
ffffffff80867a70:	02802025 	move	a0,s4
ffffffff80867a74:	de640000 	ld	a0,0(s3)
ffffffff80867a78:	de650008 	ld	a1,8(s3)
ffffffff80867a7c:	0095102b 	sltu	v0,a0,s5
ffffffff80867a80:	14400006 	bnez	v0,ffffffff80867a9c <plat_mem_setup+0x160>
ffffffff80867a84:	0085982d 	daddu	s3,a0,a1
ffffffff80867a88:	0213102b 	sltu	v0,s0,s3
ffffffff80867a8c:	1440006a 	bnez	v0,ffffffff80867c38 <plat_mem_setup+0x2fc>
ffffffff80867a90:	0090102b 	sltu	v0,a0,s0
ffffffff80867a94:	1000ffe2 	b	ffffffff80867a20 <plat_mem_setup+0xe4>
ffffffff80867a98:	66520080 	daddiu	s2,s2,128
ffffffff80867a9c:	02b3102b 	sltu	v0,s5,s3
ffffffff80867aa0:	1040000b 	beqz	v0,ffffffff80867ad0 <plat_mem_setup+0x194>
ffffffff80867aa4:	0090102b 	sltu	v0,a0,s0
ffffffff80867aa8:	02a4b82f 	dsubu	s7,s5,a0
ffffffff80867aac:	24060001 	li	a2,1
ffffffff80867ab0:	02e02825 	move	a1,s7
ffffffff80867ab4:	0c21b3fa 	jal	ffffffff8086cfe8 <add_memory_region>
ffffffff80867ab8:	0237882d 	daddu	s1,s1,s7
ffffffff80867abc:	0213102b 	sltu	v0,s0,s3
ffffffff80867ac0:	14400008 	bnez	v0,ffffffff80867ae4 <plat_mem_setup+0x1a8>
ffffffff80867ac4:	0270282f 	dsubu	a1,s3,s0
ffffffff80867ac8:	1000ffd5 	b	ffffffff80867a20 <plat_mem_setup+0xe4>
ffffffff80867acc:	66520080 	daddiu	s2,s2,128
ffffffff80867ad0:	10400005 	beqz	v0,ffffffff80867ae8 <plat_mem_setup+0x1ac>
ffffffff80867ad4:	0213102b 	sltu	v0,s0,s3
ffffffff80867ad8:	10400004 	beqz	v0,ffffffff80867aec <plat_mem_setup+0x1b0>
ffffffff80867adc:	24060001 	li	a2,1
ffffffff80867ae0:	0270282f 	dsubu	a1,s3,s0
ffffffff80867ae4:	02002025 	move	a0,s0
ffffffff80867ae8:	24060001 	li	a2,1
ffffffff80867aec:	0c21b3fa 	jal	ffffffff8086cfe8 <add_memory_region>
ffffffff80867af0:	0225882d 	daddu	s1,s1,a1
ffffffff80867af4:	1000ffca 	b	ffffffff80867a20 <plat_mem_setup+0xe4>
ffffffff80867af8:	66520080 	daddiu	s2,s2,128
ffffffff80867afc:	24100001 	li	s0,1
ffffffff80867b00:	de826218 	ld	v0,25112(s4)
ffffffff80867b04:	001090bc 	dsll32	s2,s0,0x2
ffffffff80867b08:	66430001 	daddiu	v1,s2,1
ffffffff80867b0c:	3c040040 	lui	a0,0x40
ffffffff80867b10:	0010803c 	dsll32	s0,s0,0x0
ffffffff80867b14:	2415000f 	li	s5,15
ffffffff80867b18:	0222902b 	sltu	s2,s1,v0
ffffffff80867b1c:	0044982b 	sltu	s3,v0,a0
ffffffff80867b20:	0052880a 	movz	s1,v0,s2
ffffffff80867b24:	0053200b 	movn	a0,v0,s3
ffffffff80867b28:	3c021000 	lui	v0,0x1000
ffffffff80867b2c:	6610ffff 	daddiu	s0,s0,-1
ffffffff80867b30:	3c17808f 	lui	s7,0x808f
ffffffff80867b34:	2416ffff 	li	s6,-1
ffffffff80867b38:	0223902b 	sltu	s2,s1,v1
ffffffff80867b3c:	3c032000 	lui	v1,0x2000
ffffffff80867b40:	0072100a 	movz	v0,v1,s2
ffffffff80867b44:	00809825 	move	s3,a0
ffffffff80867b48:	00008825 	move	s1,zero
ffffffff80867b4c:	0015af38 	dsll	s5,s5,0x1c
ffffffff80867b50:	0c040ab2 	jal	ffffffff80102ac8 <cvmx_bootmem_lock>
ffffffff80867b54:	00409025 	move	s2,v0
ffffffff80867b58:	8ee2b078 	lw	v0,-20360(s7)
ffffffff80867b5c:	28420020 	slti	v0,v0,32
ffffffff80867b60:	14400005 	bnez	v0,ffffffff80867b78 <plat_mem_setup+0x23c>
ffffffff80867b64:	00000000 	nop
ffffffff80867b68:	0c040ac0 	jal	ffffffff80102b00 <cvmx_bootmem_unlock>
ffffffff80867b6c:	00000000 	nop
ffffffff80867b70:	10000023 	b	ffffffff80867c00 <plat_mem_setup+0x2c4>
ffffffff80867b74:	3c02808e 	lui	v0,0x808e
ffffffff80867b78:	de826218 	ld	v0,25112(s4)
ffffffff80867b7c:	0222102b 	sltu	v0,s1,v0
ffffffff80867b80:	1040fff9 	beqz	v0,ffffffff80867b68 <plat_mem_setup+0x22c>
ffffffff80867b84:	24080002 	li	a4,2
ffffffff80867b88:	0232102b 	sltu	v0,s1,s2
ffffffff80867b8c:	02c2800a 	movz	s0,s6,v0
ffffffff80867b90:	3c070010 	lui	a3,0x10
ffffffff80867b94:	00002825 	move	a1,zero
ffffffff80867b98:	02602025 	move	a0,s3
ffffffff80867b9c:	0c04024c 	jal	ffffffff80100930 <cvmx_bootmem_phy_alloc>
ffffffff80867ba0:	02003025 	move	a2,s0
ffffffff80867ba4:	04400012 	bltz	v0,ffffffff80867bf0 <plat_mem_setup+0x2b4>
ffffffff80867ba8:	ffa20008 	sd	v0,8(sp)
ffffffff80867bac:	02a02025 	move	a0,s5
ffffffff80867bb0:	67a50008 	daddiu	a1,sp,8
ffffffff80867bb4:	ffb30000 	sd	s3,0(sp)
ffffffff80867bb8:	0c219b3c 	jal	ffffffff80866cf0 <memory_exclude_page>
ffffffff80867bbc:	03a03025 	move	a2,sp
ffffffff80867bc0:	24040001 	li	a0,1
ffffffff80867bc4:	67a50008 	daddiu	a1,sp,8
ffffffff80867bc8:	03a03025 	move	a2,sp
ffffffff80867bcc:	0c219b3c 	jal	ffffffff80866cf0 <memory_exclude_page>
ffffffff80867bd0:	0004203c 	dsll32	a0,a0,0x0
ffffffff80867bd4:	dfa50000 	ld	a1,0(sp)
ffffffff80867bd8:	10a00003 	beqz	a1,ffffffff80867be8 <plat_mem_setup+0x2ac>
ffffffff80867bdc:	dfa40008 	ld	a0,8(sp)
ffffffff80867be0:	0c21b3fa 	jal	ffffffff8086cfe8 <add_memory_region>
ffffffff80867be4:	24060001 	li	a2,1
ffffffff80867be8:	1000ffdb 	b	ffffffff80867b58 <plat_mem_setup+0x21c>
ffffffff80867bec:	0233882d 	daddu	s1,s1,s3
ffffffff80867bf0:	1216ffdd 	beq	s0,s6,ffffffff80867b68 <plat_mem_setup+0x22c>
ffffffff80867bf4:	00000000 	nop
ffffffff80867bf8:	1000ffd7 	b	ffffffff80867b58 <plat_mem_setup+0x21c>
ffffffff80867bfc:	2410ffff 	li	s0,-1
ffffffff80867c00:	dc427d60 	ld	v0,32096(v0)
ffffffff80867c04:	10400004 	beqz	v0,ffffffff80867c18 <plat_mem_setup+0x2dc>
ffffffff80867c08:	00000000 	nop
ffffffff80867c0c:	3c04806f 	lui	a0,0x806f
ffffffff80867c10:	0c040660 	jal	ffffffff80101980 <cvmx_bootmem_free_named>
ffffffff80867c14:	64847420 	daddiu	a0,a0,29728
ffffffff80867c18:	16200003 	bnez	s1,ffffffff80867c28 <plat_mem_setup+0x2ec>
ffffffff80867c1c:	3c04806f 	lui	a0,0x806f
ffffffff80867c20:	0c189cf7 	jal	ffffffff806273dc <panic>
ffffffff80867c24:	64847438 	daddiu	a0,a0,29752
ffffffff80867c28:	0c046162 	jal	ffffffff80118588 <cvmx_qlm_init>
ffffffff80867c2c:	00000000 	nop
ffffffff80867c30:	10000005 	b	ffffffff80867c48 <plat_mem_setup+0x30c>
ffffffff80867c34:	dfbf0058 	ld	ra,88(sp)
ffffffff80867c38:	1440ffa9 	bnez	v0,ffffffff80867ae0 <plat_mem_setup+0x1a4>
ffffffff80867c3c:	24060001 	li	a2,1
ffffffff80867c40:	1000ffaa 	b	ffffffff80867aec <plat_mem_setup+0x1b0>
ffffffff80867c44:	00000000 	nop
ffffffff80867c48:	dfb70050 	ld	s7,80(sp)
ffffffff80867c4c:	dfb60048 	ld	s6,72(sp)
ffffffff80867c50:	dfb50040 	ld	s5,64(sp)
ffffffff80867c54:	dfb40038 	ld	s4,56(sp)
ffffffff80867c58:	dfb30030 	ld	s3,48(sp)
ffffffff80867c5c:	dfb20028 	ld	s2,40(sp)
ffffffff80867c60:	dfb10020 	ld	s1,32(sp)
ffffffff80867c64:	dfb00018 	ld	s0,24(sp)
ffffffff80867c68:	03e00008 	jr	ra
ffffffff80867c6c:	67bd0060 	daddiu	sp,sp,96

Disassembly of section .exit.text:
~~~

## Disassembly: octeon_setup_smp

~~~asm

artifacts/pa220/original-kernel/pa220-original-vmlinux:     file format elf64-tradbigmips


Disassembly of section .text:

Disassembly of section .init.text:

ffffffff8086af80 <octeon_setup_smp>:
ffffffff8086af80:	3c02808c 	lui	v0,0x808c
ffffffff8086af84:	90424383 	lbu	v0,17283(v0)
ffffffff8086af88:	e8460003 	bbit1	v0,0x6,ffffffff8086af98 <octeon_setup_smp+0x18>
ffffffff8086af8c:	3c048082 	lui	a0,0x8082
ffffffff8086af90:	10000003 	b	ffffffff8086afa0 <octeon_setup_smp+0x20>
ffffffff8086af94:	64847700 	daddiu	a0,a0,30464
ffffffff8086af98:	3c048082 	lui	a0,0x8082
ffffffff8086af9c:	648476c0 	daddiu	a0,a0,30400
ffffffff8086afa0:	0822350c 	j	ffffffff8088d430 <register_smp_ops>
ffffffff8086afa4:	00000000 	nop

Disassembly of section .exit.text:
~~~

## Relevant embedded strings

~~~text
 5f3639 3Disabling rootwait; root= is invalid.
 5f36c0 Please append a correct "root=" boot option; here are the available partitions:
 5f8288  console=ttyS0,115200
 5f82f0  cpuid=0
 5f8300  cpuid=1
 5f8310  cpuid=2
 5f8320  cpuid=3
 5f8330  rw root=1f00 slram=root,0x40000000,+1073741824
 5f8589 6Using passed Device Tree <%p>.
 5f8601 6Using internal Device Tree.
 5f91a0 fdt_addr:            0x%lx
 62c771 7Mac addr in bootinfo: %x:%x:%x:%x:%x:%x
 62c858 %s: MAC from bootinfo: %02x:%02x:%02x:%02x:%02x:%02x
 671355 octeon_bootinfo
 7a5344 root=
 7a5629 nfsroot=
 8ef037 __ksymtab_octeon_bootinfo
 8ef051 __kstrtab_octeon_bootinfo
 8ef06b __kcrctab_octeon_bootinfo
 9ad6fe cvmx_sysinfo_remove_self_from_core_mask
 9cf452 __crc_octeon_bootinfo
 9e5433 cvmx_sysinfo_add_self_to_core_mask
~~~

## Interpretation boundary

- This report exposes the exact compiled kernel reads of the boot descriptor, bootinfo version/coremask/FDT fields, and SMP setup paths.
- It does not infer runtime RAM contents from U-Boot's own diagnostic prints.
- A symbol may be inlined or optimized; absence from the symbol list is not proof that the corresponding logic is absent.
- The literal string scan is not a substitute for reconstructing the actual command line.
