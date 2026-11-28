# PA-220 OEM SPI executable scan

Read-only analysis of the original SPI backup. No device access or writes.

- SPI file: /home/runner/work/_temp/pa220-spi-normal.img
- SPI size: 16777216 bytes (0x1000000)

## OEM FDT signatures

- FDT at SPI offset 0x1c9460, size 10850 (0x2a62)

## Embedded MIPS64 big-endian ELF candidates

- No structurally valid ELF64 big-endian MIPS payload found by signature scan.

## OEM Stage-3 raw image

- SPI offset: 0x100000
- Size: 1310920 bytes (0x1400c8)
- First 64 bytes: 10 00 01 3f 00 00 00 00 42 4f 4f 54 3b d6 05 52 00 c0 00 01 00 02 4e 35 00 14 00 08 10 3c 4b cb ff ff ff ff c0 00 00 00 00 00 00 00 00 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
- Header entry point: 0xc0000000
- Raw-file VMA including 64-byte header: 0xbfffffc0
- PA-220 FDT relative offset: 0xc9460
- FDT signature inside extracted image: True

### Raw MIPS64 disassembly: bootinfo-related field offsets

- Full disassembly instruction lines: 319728
- Direct load/store candidates at offsets 152/160/176/184: 503

~~~asm
    c0000df4:	00094c38 	dsll	t1,t1,0x10
    c0000df8:	fd090080 	sd	t1,128(t0)
    c0000dfc:	fd000098 	sd	zero,152(t0)
    c0000e00:	3c0ac000 	lui	t2,0xc000
    c0000e04:	254a1440 	addiu	t2,t2,5184

    c0000e1c:	012a482d 	daddu	t1,t1,t2
    c0000e20:	dd2b0000 	ld	t3,0(t1)
    c0000e24:	fd0b0098 	sd	t3,152(t0)
    c0000e28:	dd2b0008 	ld	t3,8(t1)
    c0000e2c:	fd0b0098 	sd	t3,152(t0)

    c0000e30:	dd000080 	ld	zero,128(t0)
    c0000e34:	3c088001 	lui	t0,0x8001

    c00011dc:	ff710088 	sd	s1,136(k1)
    c00011e0:	ff720090 	sd	s2,144(k1)
    c00011e4:	ff730098 	sd	s3,152(k1)
    c00011e8:	ff7400a0 	sd	s4,160(k1)
    c00011ec:	ff7500a8 	sd	s5,168(k1)

    c00011f0:	ff7600b0 	sd	s6,176(k1)

    c00011f4:	ff7700b8 	sd	s7,184(k1)
    c00011f8:	ff7800c0 	sd	t8,192(k1)

    c00011fc:	ff7900c8 	sd	t9,200(k1)

    c0001614:	ff510088 	sd	s1,136(k0)
    c0001618:	ff520090 	sd	s2,144(k0)
    c000161c:	ff530098 	sd	s3,152(k0)
    c0001620:	ff5400a0 	sd	s4,160(k0)
    c0001624:	ff5500a8 	sd	s5,168(k0)

    c0001628:	ff5600b0 	sd	s6,176(k0)

    c000162c:	ff5700b8 	sd	s7,184(k0)
    c0001630:	ff5800c0 	sd	t8,192(k0)

    c0001634:	ff5900c8 	sd	t9,200(k0)

    c000332c:	ffb300a8 	sd	s3,168(sp)
    c0003330:	00809825 	move	s3,a0
    c0003334:	ffb500b8 	sd	s5,184(sp)
    c0003338:	24040003 	li	a0,3
    c000333c:	ffbf00d8 	sd	ra,216(sp)

    c0003348:	ffb700c8 	sd	s7,200(sp)
    c000334c:	ffb600c0 	sd	s6,192(sp)
    c0003350:	ffb400b0 	sd	s4,176(sp)
    c0003354:	ffb200a0 	sd	s2,160(sp)
    c0003358:	ffb10098 	sd	s1,152(sp)

    c000335c:	0c000811 	jal	0xc0002044

    c0003360:	ffb00090 	sd	s0,144(sp)

    c0003400:	dfb700c8 	ld	s7,200(sp)
    c0003404:	dfb600c0 	ld	s6,192(sp)
    c0003408:	dfb500b8 	ld	s5,184(sp)
    c000340c:	dfb400b0 	ld	s4,176(sp)
    c0003410:	dfb300a8 	ld	s3,168(sp)

    c0003414:	dfb200a0 	ld	s2,160(sp)

    c0003418:	dfb10098 	ld	s1,152(sp)
    c000341c:	dfb00090 	ld	s0,144(sp)

    c0003420:	03e00008 	jr	ra

    c000439c:	ffa00088 	sd	zero,136(sp)
    c00043a0:	7c024204 	ins	v0,zero,0x8,0x1
    c00043a4:	ffa000a0 	sd	zero,160(sp)
    c00043a8:	7c0239c4 	ins	v0,zero,0x7,0x1
    c00043ac:	ffa000b0 	sd	zero,176(sp)

    c00043b0:	7c023184 	ins	v0,zero,0x6,0x1
    c00043b4:	ffa000b8 	sd	zero,184(sp)

    c00043b8:	7c622944 	ins	v0,v1,0x5,0x1
    c00043bc:	a7a200d8 	sh	v0,216(sp)

    c00043cc:	ffa20090 	sd	v0,144(sp)
    c00043d0:	24020080 	li	v0,128
    c00043d4:	ffa20098 	sd	v0,152(sp)
    c00043d8:	2402ffff 	li	v0,-1
    c00043dc:	ffa30080 	sd	v1,128(sp)

    c0004fd0:	ffbf00a8 	sd	ra,168(sp)
    c0004fd4:	7c930e02 	dext	s3,a0,0x38,0x2
    c0004fd8:	ffbe00a0 	sd	s8,160(sp)
    c0004fdc:	ffb70098 	sd	s7,152(sp)
    c0004fe0:	ffb50088 	sd	s5,136(sp)

    c0004fe4:	ffb40080 	sd	s4,128(sp)

    c0004ffc:	2402ffff 	li	v0,-1
    c0005000:	dfbf00a8 	ld	ra,168(sp)
    c0005004:	dfbe00a0 	ld	s8,160(sp)
    c0005008:	dfb70098 	ld	s7,152(sp)
    c000500c:	dfb60090 	ld	s6,144(sp)

    c0005010:	dfb50088 	ld	s5,136(sp)

    c000b178:	27bdff30 	addiu	sp,sp,-208
    c000b17c:	ffbf00c8 	sd	ra,200(sp)
    c000b180:	ffb30098 	sd	s3,152(sp)
    c000b184:	ffb20090 	sd	s2,144(sp)
    c000b188:	3c12010d 	lui	s2,0x10d

    c000b190:	ffb00080 	sd	s0,128(sp)
    c000b194:	ffbe00c0 	sd	s8,192(sp)
    c000b198:	ffb700b8 	sd	s7,184(sp)
    c000b19c:	ffb600b0 	sd	s6,176(sp)
    c000b1a0:	ffb500a8 	sd	s5,168(sp)

    c000b1a4:	0c0021c2 	jal	0xc0008708

    c000b1a8:	ffb400a0 	sd	s4,160(sp)
    c000b1ac:	36449508 	ori	a0,s2,0x9508
    c000b1b0:	afa20018 	sw	v0,24(sp)

    c000b210:	8fa20010 	lw	v0,16(sp)
    c000b214:	dfbe00c0 	ld	s8,192(sp)
    c000b218:	dfb700b8 	ld	s7,184(sp)
    c000b21c:	dfb600b0 	ld	s6,176(sp)
    c000b220:	dfb500a8 	ld	s5,168(sp)

    c000b224:	dfb400a0 	ld	s4,160(sp)

    c000b228:	dfb30098 	ld	s3,152(sp)
    c000b22c:	dfb20090 	ld	s2,144(sp)

    c000b230:	dfb10088 	ld	s1,136(sp)

    c001a390:	ffb00050 	sd	s0,80(sp)
    c001a394:	00028202 	srl	s0,v0,0x8
    c001a398:	ffbf0098 	sd	ra,152(sp)
    c001a39c:	7e140880 	ext	s4,s0,0x2,0x2
    c001a3a0:	ffbe0090 	sd	s8,144(sp)

    c001c308:	14401184 	bnez	v0,0xc002091c
    c001c30c:	afa20008 	sw	v0,8(sp)
    c001c310:	dfbf0098 	ld	ra,152(sp)
    c001c314:	02201025 	move	v0,s1
    c001c318:	dfbe0090 	ld	s8,144(sp)

    c001d95c:	0062102b 	sltu	v0,v1,v0
    c001d960:	1440fa6c 	bnez	v0,0xc001c314
    c001d964:	dfbf0098 	ld	ra,152(sp)
    c001d968:	7c03f83b 	rdhwr	v1,$31
    c001d96c:	64630064 	daddiu	v1,v1,100

    c001da3c:	0052102b 	sltu	v0,v0,s2
    c001da40:	1440099f 	bnez	v0,0xc00200c0
    c001da44:	dfbf0098 	ld	ra,152(sp)
    c001da48:	1000fa33 	b	0xc001c318
    c001da4c:	02201025 	move	v0,s1

    c0020b14:	24840300 	addiu	a0,a0,768
    c0020b18:	1440edfe 	bnez	v0,0xc001c314
    c0020b1c:	dfbf0098 	ld	ra,152(sp)
    c0020b20:	3c12010d 	lui	s2,0x10d
    c0020b24:	0c00b38b 	jal	0xc002ce2c

    c0020b28:	26440308 	addiu	a0,s2,776
    c0020b2c:	1440edf9 	bnez	v0,0xc001c314
    c0020b30:	dfbf0098 	ld	ra,152(sp)
    c0020b34:	0c00b38b 	jal	0xc002ce2c
    c0020b38:	26440600 	addiu	a0,s2,1536

    c0020b3c:	1440edf5 	bnez	v0,0xc001c314
    c0020b40:	dfbf0098 	ld	ra,152(sp)
    c0020b44:	0c00b38b 	jal	0xc002ce2c
    c0020b48:	36449508 	ori	a0,s2,0x9508

    c0020bc0:	00000000 	nop
    c0020bc4:	c850edd3 	lwc2	$16,-4653(v0)
    c0020bc8:	dfbf0098 	ld	ra,152(sp)
    c0020bcc:	e865edd2 	swc2	$5,-4654(v1)
    c0020bd0:	02201025 	move	v0,s1

    c0023d6c:	ffb40080 	sd	s4,128(sp)
    c0023d70:	0080a025 	move	s4,a0
    c0023d74:	ffbf0098 	sd	ra,152(sp)
    c0023d78:	ffb60090 	sd	s6,144(sp)
    c0023d7c:	ffb30078 	sd	s3,120(sp)

    c0023d90:	944200c0 	lhu	v0,192(v0)
    c0023d94:	1455000d 	bne	v0,s5,0xc0023dcc
    c0023d98:	dfbf0098 	ld	ra,152(sp)
    c0023d9c:	3c10000c 	lui	s0,0xc
    c0023da0:	361003e8 	ori	s0,s0,0x3e8

    c0023e5c:	02402025 	move	a0,s2
    c0023e60:	1000ffd0 	b	0xc0023da4
    c0023e64:	dfbf0098 	ld	ra,152(sp)
    c0023e68:	3c04010d 	lui	a0,0x10d
    c0023e6c:	0c00b38b 	jal	0xc002ce2c

    c002400c:	361003e8 	ori	s0,s0,0x3e8
    c0024010:	0682ff64 	bltzl	s4,0xc0023da4
    c0024014:	dfbf0098 	ld	ra,152(sp)
    c0024018:	1000ff8e 	b	0xc0023e54
    c002401c:	02003025 	move	a2,s0

    c0024020:	0440ff60 	bltz	v0,0xc0023da4
    c0024024:	dfbf0098 	ld	ra,152(sp)
    c0024028:	8e220010 	lw	v0,16(s1)
    c002402c:	54400004 	bnezl	v0,0xc0024040

    c00240a4:	00808825 	move	s1,a0
    c00240a8:	ffbf00a8 	sd	ra,168(sp)
    c00240ac:	ffb700a0 	sd	s7,160(sp)
    c00240b0:	ffb60098 	sd	s6,152(sp)
    c00240b4:	ffb50090 	sd	s5,144(sp)

    c00240b8:	ffb40088 	sd	s4,136(sp)

    c00240fc:	02202025 	move	a0,s1
    c0024100:	dfbf00a8 	ld	ra,168(sp)
    c0024104:	dfb700a0 	ld	s7,160(sp)
    c0024108:	dfb60098 	ld	s6,152(sp)
    c002410c:	dfb50090 	ld	s5,144(sp)

    c0024110:	dfb40088 	ld	s4,136(sp)

    c0024fa4:	3c02c015 	lui	v0,0xc015
    c0024fa8:	03e00008 	jr	ra
    c0024fac:	8c427da0 	lw	v0,32160(v0)
    c0024fb0:	14800003 	bnez	a0,0xc0024fc0
    c0024fb4:	2402ffff 	li	v0,-1

    c0025538:	dfb10018 	ld	s1,24(sp)
    c002553c:	dfb00010 	ld	s0,16(sp)
    c0025540:	ac527da0 	sw	s2,32160(v0)
    c0025544:	dfb20020 	ld	s2,32(sp)
    c0025548:	03e00008 	jr	ra

    c00263e0:	ffb10010 	sd	s1,16(sp)
    c00263e4:	3c11c015 	lui	s1,0xc015
    c00263e8:	8e227d98 	lw	v0,32152(s1)
    c00263ec:	ffbf0038 	sd	ra,56(sp)
    c00263f0:	ffb50030 	sd	s5,48(sp)

    c0026414:	0c000d4f 	jal	0xc000353c
    c0026418:	dc440120 	ld	a0,288(v0)
    c002641c:	ae227d98 	sw	v0,32152(s1)
    c0026420:	3c10010d 	lui	s0,0x10d
    c0026424:	0c00b38b 	jal	0xc002ce2c

    c0026450:	00408025 	move	s0,v0
    c0026454:	0c008671 	jal	0xc00219c4
    c0026458:	8e247d98 	lw	a0,32152(s1)
    c002645c:	14400009 	bnez	v0,0xc0026484
    c0026460:	00408025 	move	s0,v0

    c0026474:	00408025 	move	s0,v0
    c0026478:	0c0086e0 	jal	0xc0021b80
    c002647c:	8e247d98 	lw	a0,32152(s1)
    c0026480:	00408025 	move	s0,v0
    c0026484:	24118001 	li	s1,-32767

    c00265e0:	0c00a2af 	jal	0xc0028abc
    c00265e4:	02402025 	move	a0,s2
    c00265e8:	8e247d98 	lw	a0,32152(s1)
    c00265ec:	0c008890 	jal	0xc0022240
    c00265f0:	00402825 	move	a1,v0

    c0026960:	ace20000 	sw	v0,0(a3)
    c0026964:	27bdff40 	addiu	sp,sp,-192
    c0026968:	ffb50098 	sd	s5,152(sp)
    c002696c:	3c15c014 	lui	s5,0xc014
    c0026970:	ffb30088 	sd	s3,136(sp)

    c002698c:	ffa20018 	sd	v0,24(sp)
    c0026990:	00029000 	sll	s2,v0,0x0
    c0026994:	ffbf00b8 	sd	ra,184(sp)
    c0026998:	00111438 	dsll	v0,s1,0x10
    c002699c:	ffbe00b0 	sd	s8,176(sp)

    c00269a0:	64420001 	daddiu	v0,v0,1
    c00269a4:	ffb700a8 	sd	s7,168(sp)

    c00269a8:	000216f8 	dsll	v0,v0,0x1b
    c00269ac:	ffb600a0 	sd	s6,160(sp)
    c00269b0:	64420010 	daddiu	v0,v0,16
    c00269b4:	ffb40090 	sd	s4,144(sp)

    c0026f70:	00002025 	move	a0,zero
    c0026f74:	0000000f 	sync
    c0026f78:	dfbf00b8 	ld	ra,184(sp)
    c0026f7c:	0014102b 	sltu	v0,zero,s4
    c0026f80:	dfbe00b0 	ld	s8,176(sp)

    c0026f84:	dfb700a8 	ld	s7,168(sp)
    c0026f88:	dfb600a0 	ld	s6,160(sp)

    c0026f8c:	dfb50098 	ld	s5,152(sp)
    c0026f90:	dfb40090 	ld	s4,144(sp)

    c0026f94:	dfb30088 	ld	s3,136(sp)

    c002aebc:	fc550008 	sd	s5,8(v0)
    c002aec0:	a45400c0 	sh	s4,192(v0)
    c002aec4:	ac5100b8 	sw	s1,184(v0)
    c002aec8:	24020001 	li	v0,1
    c002aecc:	dfb50030 	ld	s5,48(sp)

    c002b9f4:	00c08025 	move	s0,a2
    c002b9f8:	afa40004 	sw	a0,4(sp)
    c002b9fc:	ffbf0098 	sd	ra,152(sp)
    c002ba00:	ffb70088 	sd	s7,136(sp)
    c002ba04:	ffb60080 	sd	s6,128(sp)

    c002ba38:	2402ffff 	li	v0,-1
    c002ba3c:	afa20000 	sw	v0,0(sp)
    c002ba40:	dfbf0098 	ld	ra,152(sp)
    c002ba44:	8fa20000 	lw	v0,0(sp)
    c002ba48:	dfbe0090 	ld	s8,144(sp)

    c002c170:	ffb40070 	sd	s4,112(sp)
    c002c174:	00e0a025 	move	s4,a3
    c002c178:	ffbf0098 	sd	ra,152(sp)
    c002c17c:	ffbe0090 	sd	s8,144(sp)
    c002c180:	ffb70088 	sd	s7,136(sp)

    c002c1b4:	3c04fff0 	lui	a0,0xfff0
    c002c1b8:	2412ffff 	li	s2,-1
    c002c1bc:	dfbf0098 	ld	ra,152(sp)
    c002c1c0:	02401025 	move	v0,s2
    c002c1c4:	dfbe0090 	ld	s8,144(sp)

    c002c74c:	fe820000 	sd	v0,0(s4)
    c002c750:	27bdff30 	addiu	sp,sp,-208
    c002c754:	ffb600b0 	sd	s6,176(sp)
    c002c758:	0080b025 	move	s6,a0
    c002c75c:	ffb10088 	sd	s1,136(sp)

    c002c76c:	ffbf00c8 	sd	ra,200(sp)
    c002c770:	ffbe00c0 	sd	s8,192(sp)
    c002c774:	ffb700b8 	sd	s7,184(sp)
    c002c778:	ffb500a8 	sd	s5,168(sp)
    c002c77c:	ffb400a0 	sd	s4,160(sp)

    c002c780:	ffb30098 	sd	s3,152(sp)
    c002c784:	ffb20090 	sd	s2,144(sp)

    c002c788:	afa5004c 	sw	a1,76(sp)

    c002c7b8:	8fa20010 	lw	v0,16(sp)
    c002c7bc:	dfbe00c0 	ld	s8,192(sp)
    c002c7c0:	dfb700b8 	ld	s7,184(sp)
    c002c7c4:	dfb600b0 	ld	s6,176(sp)
    c002c7c8:	dfb500a8 	ld	s5,168(sp)

    c002c7cc:	dfb400a0 	ld	s4,160(sp)

    c002c7d0:	dfb30098 	ld	s3,152(sp)
    c002c7d4:	dfb20090 	ld	s2,144(sp)

    c002c7d8:	dfb10088 	ld	s1,136(sp)

    c002ebb8:	ffb200c0 	sd	s2,192(sp)
    c002ebbc:	00c09025 	move	s2,a2
    c002ebc0:	ffb100b8 	sd	s1,184(sp)
    c002ebc4:	00808825 	move	s1,a0
    c002ebc8:	ffb000b0 	sd	s0,176(sp)

    c002ebcc:	ffbe00f0 	sd	s8,240(sp)
    c002ebd0:	ffb700e8 	sd	s7,232(sp)

    c002eca0:	dfb300c8 	ld	s3,200(sp)
    c002eca4:	dfb200c0 	ld	s2,192(sp)
    c002eca8:	dfb100b8 	ld	s1,184(sp)
    c002ecac:	dfb000b0 	ld	s0,176(sp)
    c002ecb0:	0800b40d 	j	0xc002d034

    c002ecb4:	27bd0100 	addiu	sp,sp,256

    c002ece0:	afa800a4 	sw	t0,164(sp)
    c002ece4:	0c00b38b 	jal	0xc002ce2c
    c002ece8:	afa600a0 	sw	a2,160(sp)
    c002ecec:	03c01825 	move	v1,s8
    c002ecf0:	02a03825 	move	a3,s5

    c002ecf4:	8e640004 	lw	a0,4(s3)
    c002ecf8:	02c2180a 	movz	v1,s6,v0
    c002ecfc:	8fa600a0 	lw	a2,160(sp)
    c002ed00:	8fa800a4 	lw	t0,164(sp)
    c002ed04:	8fa900a8 	lw	t1,168(sp)

    c002edb8:	dfb300c8 	ld	s3,200(sp)
    c002edbc:	dfb200c0 	ld	s2,192(sp)
    c002edc0:	dfb100b8 	ld	s1,184(sp)
    c002edc4:	dfb000b0 	ld	s0,176(sp)
    c002edc8:	03e00008 	jr	ra

    c002edcc:	27bd0100 	addiu	sp,sp,256

    c0032b6c:	00000000 	nop
    c0032b70:	7ea20c00 	ext	v0,s5,0x10,0x2
    c0032b74:	afa200b0 	sw	v0,176(sp)
    c0032b78:	3c10010d 	lui	s0,0x10d
    c0032b7c:	0c00b38b 	jal	0xc002ce2c

    c0032cb4:	3c06c015 	lui	a2,0xc015
    c0032cb8:	00002025 	move	a0,zero
    c0032cbc:	afa600b8 	sw	a2,184(sp)
    c0032cc0:	00001025 	move	v0,zero
    c0032cc4:	acc57dd8 	sw	a1,32216(a2)

    c0032ce4:	10800003 	beqz	a0,0xc0032cf4
    c0032ce8:	00b08021 	addu	s0,a1,s0
    c0032cec:	8fa200b8 	lw	v0,184(sp)
    c0032cf0:	ac507dd8 	sw	s0,32216(v0)
    c0032cf4:	02778021 	addu	s0,s3,s7

    c0032d08:	8e437de0 	lw	v1,32224(s2)
    c0032d0c:	8e020000 	lw	v0,0(s0)
    c0032d10:	8fa400b0 	lw	a0,176(sp)
    c0032d14:	8e9e7de4 	lw	s8,32228(s4)
    c0032d18:	00431021 	addu	v0,v0,v1

    c0032d74:	24050008 	li	a1,8
    c0032d78:	0c00093b 	jal	0xc00024ec
    c0032d7c:	afa600b8 	sw	a2,184(sp)
    c0032d80:	8fa600b8 	lw	a2,184(sp)
    c0032d84:	1440ffc7 	bnez	v0,0xc0032ca4

    c0032d88:	aec27ddc 	sw	v0,32220(s6)

    c0032f14:	8fa200c0 	lw	v0,192(sp)
    c0032f18:	ac447dd0 	sw	a0,32208(v0)
    c0032f1c:	8fa200b0 	lw	v0,176(sp)
    c0032f20:	24070001 	li	a3,1
    c0032f24:	afa300bc 	sw	v1,188(sp)

    c0032f28:	00021100 	sll	v0,v0,0x4
    c0032f2c:	00511025 	or	v0,v0,s1
    c0032f30:	afa200b0 	sw	v0,176(sp)
    c0032f34:	0c00fb42 	jal	0xc003ed08
    c0032f38:	8fa400b0 	lw	a0,176(sp)

    c0032f3c:	0441000d 	bgez	v0,0xc0032f74
    c0032f40:	8fa300bc 	lw	v1,188(sp)

    c0032f88:	afa300bc 	sw	v1,188(sp)
    c0032f8c:	8ec57ddc 	lw	a1,32220(s6)
    c0032f90:	8fa200b8 	lw	v0,184(sp)
    c0032f94:	00004825 	move	t1,zero
    c0032f98:	8fc47dcc 	lw	a0,32204(s8)

    c0032fd4:	11200004 	beqz	t1,0xc0032fe8
    c0032fd8:	00a21021 	addu	v0,a1,v0
    c0032fdc:	8fa500b8 	lw	a1,184(sp)
    c0032fe0:	ac647dc8 	sw	a0,32200(v1)
    c0032fe4:	aca27dd8 	sw	v0,32216(a1)

    c0033020:	8e650000 	lw	a1,0(s3)
    c0033024:	0c00fa69 	jal	0xc003e9a4
    c0033028:	8fa400b0 	lw	a0,176(sp)
    c003302c:	0441ff03 	bgez	v0,0xc0032c3c
    c0033030:	00000000 	nop

    c0033104:	027e9825 	or	s3,s3,s8
    c0033108:	03d7b825 	or	s7,s8,s7
    c003310c:	ffa200b0 	sd	v0,176(sp)
    c0033110:	0014a378 	dsll	s4,s4,0xd
    c0033114:	02d0102a 	slt	v0,s6,s0

    c003316c:	0c00a336 	jal	0xc0028cd8
    c0033170:	02a02025 	move	a0,s5
    c0033174:	dfa500b0 	ld	a1,176(sp)
    c0033178:	32c300ff 	andi	v1,s6,0xff
    c003317c:	00402025 	move	a0,v0

    c00331a4:	00002025 	move	a0,zero
    c00331a8:	0c0104b4 	jal	0xc00412d0
    c00331ac:	afa200b8 	sw	v0,184(sp)
    c00331b0:	8fa200b8 	lw	v0,184(sp)
    c00331b4:	24030008 	li	v1,8

    c00331b8:	03a03825 	move	a3,sp

    c0034c1c:	ffb00068 	sd	s0,104(sp)
    c0034c20:	00002825 	move	a1,zero
    c0034c24:	ffbf0098 	sd	ra,152(sp)
    c0034c28:	00808025 	move	s0,a0
    c0034c2c:	ffb50090 	sd	s5,144(sp)

    c0034e8c:	24110019 	li	s1,25
    c0034e90:	00001025 	move	v0,zero
    c0034e94:	dfbf0098 	ld	ra,152(sp)
    c0034e98:	dfb50090 	ld	s5,144(sp)
    c0034e9c:	dfb40088 	ld	s4,136(sp)

    c0038f8c:	ffb700d8 	sd	s7,216(sp)
    c0038f90:	ffb600d0 	sd	s6,208(sp)
    c0038f94:	ffb300b8 	sd	s3,184(sp)
    c0038f98:	ffb200b0 	sd	s2,176(sp)
    c0038f9c:	0c00a380 	jal	0xc0028e00

    c0038fa0:	ffb000a0 	sd	s0,160(sp)

    c0038fa4:	02a02025 	move	a0,s5
    c0038fa8:	0c00a40a 	jal	0xc0029028

    c0039024:	ffa20088 	sd	v0,136(sp)
    c0039028:	32220001 	andi	v0,s1,0x1
    c003902c:	afa20098 	sw	v0,152(sp)
    c0039030:	3c02fffc 	lui	v0,0xfffc
    c0039034:	64420009 	daddiu	v0,v0,9

    c0039630:	30460003 	andi	a2,v0,0x3
    c0039634:	65080001 	daddiu	t0,t0,1
    c0039638:	8fa20098 	lw	v0,152(sp)
    c003963c:	00021438 	dsll	v0,v0,0x10
    c0039640:	00c2302d 	daddu	a2,a2,v0

    c0039b10:	dc420000 	ld	v0,0(v0)
    c0039b14:	3c08010d 	lui	t0,0x10d
    c0039b18:	afa80098 	sw	t0,152(sp)
    c0039b1c:	0c00b38b 	jal	0xc002ce2c
    c0039b20:	25040200 	addiu	a0,t0,512

    c0039b24:	10400032 	beqz	v0,0xc0039bf0
    c0039b28:	8fa80098 	lw	t0,152(sp)
    c0039b2c:	2402000a 	li	v0,10
    c0039b30:	1682004b 	bne	s4,v0,0xc0039c60

    c003a098:	dfb500c8 	ld	s5,200(sp)
    c003a09c:	dfb400c0 	ld	s4,192(sp)
    c003a0a0:	dfb300b8 	ld	s3,184(sp)
    c003a0a4:	dfb200b0 	ld	s2,176(sp)
    c003a0a8:	dfb100a8 	ld	s1,168(sp)

    c003a0ac:	dfb000a0 	ld	s0,160(sp)

    c003a0b0:	03e00008 	jr	ra
    c003a0b4:	27bd00f0 	addiu	sp,sp,240

    c003f584:	7c650240 	ext	a1,v1,0x9,0x1
    c003f588:	8cc20008 	lw	v0,8(a2)
    c003f58c:	dfa700a0 	ld	a3,160(sp)
    c003f590:	ffa80050 	sd	t0,80(sp)
    c003f594:	ffa90058 	sd	t1,88(sp)

    c003f5b4:	dfb10088 	ld	s1,136(sp)
    c003f5b8:	7c6402c0 	ext	a0,v1,0xb,0x1
    c003f5bc:	dfb20098 	ld	s2,152(sp)
    c003f5c0:	00852025 	or	a0,a0,a1
    c003f5c4:	7c827bc4 	ins	v0,a0,0xf,0x1

    c003f720:	ffaa0090 	sd	t2,144(sp)
    c003f724:	0c018cf3 	jal	0xc00633cc
    c003f728:	ffab0098 	sd	t3,152(sp)
    c003f72c:	0c00b38b 	jal	0xc002ce2c
    c003f730:	36049508 	ori	a0,s0,0x9508

    c003f760:	dfaa0090 	ld	t2,144(sp)
    c003f764:	0c00fd53 	jal	0xc003f54c
    c003f768:	dfab0098 	ld	t3,152(sp)
    c003f76c:	dfbf0058 	ld	ra,88(sp)
    c003f770:	00001025 	move	v0,zero

    c00418a8:	afa90038 	sw	t1,56(sp)
    c00418ac:	28a90180 	slti	t1,a1,384
    c00418b0:	ffbf0098 	sd	ra,152(sp)
    c00418b4:	ffbe0090 	sd	s8,144(sp)
    c00418b8:	ffb70088 	sd	s7,136(sp)

    c0041958:	dfa30048 	ld	v1,72(sp)
    c004195c:	00001025 	move	v0,zero
    c0041960:	dfbf0098 	ld	ra,152(sp)
    c0041964:	dfbe0090 	ld	s8,144(sp)
    c0041968:	dfb70088 	ld	s7,136(sp)

    c0041a74:	01545025 	or	t2,t2,s4
    c0041a78:	dd420000 	ld	v0,0(t2)
    c0041a7c:	8fab00a0 	lw	t3,160(sp)
    c0041a80:	316b007f 	andi	t3,t3,0x7f
    c0041a84:	7d62f607 	dins	v0,t3,0x18,0x7

    c0041aa0:	316b001f 	andi	t3,t3,0x1f
    c0041aa4:	7d626207 	dins	v0,t3,0x8,0x5
    c0041aa8:	8fab00b0 	lw	t3,176(sp)
    c0041aac:	7d623807 	dins	v0,t3,0x0,0x8
    c0041ab0:	fd420000 	sd	v0,0(t2)

    c0044118:	36249700 	ori	a0,s1,0x9700
    c004411c:	0002180a 	movz	v1,zero,v0
    c0044120:	afa300b8 	sw	v1,184(sp)
    c0044124:	2403001c 	li	v1,28
    c0044128:	0002180a 	movz	v1,zero,v0

    c0044134:	1040005c 	beqz	v0,0xc00442a8
    c0044138:	24020005 	li	v0,5
    c004413c:	afa200b8 	sw	v0,184(sp)
    c0044140:	24020010 	li	v0,16
    c0044144:	afa200c0 	sw	v0,192(sp)

    c0044268:	54400153 	bnezl	v0,0xc00447b8
    c004426c:	02809825 	move	s3,s4
    c0044270:	8fa200b8 	lw	v0,184(sp)
    c0044274:	3c16c00e 	lui	s6,0xc00e
    c0044278:	3c17c00e 	lui	s7,0xc00e

    c0044294:	26d68788 	addiu	s6,s6,-30840
    c0044298:	26f784e0 	addiu	s7,s7,-31520
    c004429c:	afa000b0 	sw	zero,176(sp)
    c00442a0:	10000154 	b	0xc00447f4
    c00442a4:	02808825 	move	s1,s4

    c00442b0:	1440ffa2 	bnez	v0,0xc004413c
    c00442b4:	24020005 	li	v0,5
    c00442b8:	8fa200b8 	lw	v0,184(sp)
    c00442bc:	1440ffa3 	bnez	v0,0xc004414c
    c00442c0:	24060008 	li	a2,8

    c00447a4:	0017b842 	srl	s7,s7,0x1
    c00447a8:	12e0feb2 	beqz	s7,0xc0044274
    c00447ac:	8fa200b8 	lw	v0,184(sp)
    c00447b0:	1000feab 	b	0xc0044260
    c00447b4:	241e0001 	li	s8,1

    c00447dc:	a04400a8 	sb	a0,168(v0)
    c00447e0:	03d2f021 	addu	s8,s8,s2
    c00447e4:	8fa200b0 	lw	v0,176(sp)
    c00447e8:	26310005 	addiu	s1,s1,5
    c00447ec:	24420001 	addiu	v0,v0,1

    c00447f0:	afa200b0 	sw	v0,176(sp)
    c00447f4:	0c00b38b 	jal	0xc002ce2c
    c00447f8:	36a49800 	ori	a0,s5,0x9800

    c00448b0:	1000ffcc 	b	0xc00447e4
    c00448b4:	03d2f021 	addu	s8,s8,s2
    c00448b8:	8fa200b0 	lw	v0,176(sp)
    c00448bc:	0044202b 	sltu	a0,v0,a0
    c00448c0:	5080ffd9 	beqzl	a0,0xc0044828

    c00448e8:	a23e0001 	sb	s8,1(s1)
    c00448ec:	03a21021 	addu	v0,sp,v0
    c00448f0:	904400a0 	lbu	a0,160(v0)
    c00448f4:	00852021 	addu	a0,a0,a1
    c00448f8:	a04400a0 	sb	a0,160(v0)

    c00448fc:	24040004 	li	a0,4
    c0044900:	5644ffb0 	bnel	s2,a0,0xc00447c4

    c004497c:	14600081 	bnez	v1,0xc0044b84
    c0044980:	02201025 	move	v0,s1
    c0044984:	ffa500b0 	sd	a1,176(sp)
    c0044988:	0c00b38b 	jal	0xc002ce2c
    c004498c:	02c02025 	move	a0,s6

    c0044990:	10400075 	beqz	v0,0xc0044b68
    c0044994:	dfa500b0 	ld	a1,176(sp)
    c0044998:	02201025 	move	v0,s1
    c004499c:	24030004 	li	v1,4

    c00449a8:	7c023187 	dins	v0,zero,0x6,0x1
    c00449ac:	fea20000 	sd	v0,0(s5)
    c00449b0:	8fa200b8 	lw	v0,184(sp)
    c00449b4:	26730001 	addiu	s3,s3,1
    c00449b8:	1453ffdf 	bne	v0,s3,0xc0044938

    c0044a44:	00118cf8 	dsll	s1,s1,0x13
    c0044a48:	26d68788 	addiu	s6,s6,-30840
    c0044a4c:	afa200b0 	sw	v0,176(sp)
    c0044a50:	26f78550 	addiu	s7,s7,-31408
    c0044a54:	3c12010d 	lui	s2,0x10d

    c0044acc:	10520006 	beq	v0,s2,0xc0044ae8
    c0044ad0:	00402825 	move	a1,v0
    c0044ad4:	8fa200b0 	lw	v0,176(sp)
    c0044ad8:	02c02825 	move	a1,s6
    c0044adc:	0c023325 	jal	0xc008cc94

    c0044ae0:	24448528 	addiu	a0,v0,-31448
    c0044ae4:	24050fff 	li	a1,4095
    c0044ae8:	ffa500b8 	sd	a1,184(sp)
    c0044aec:	3c04080d 	lui	a0,0x80d
    c0044af0:	0c00b38b 	jal	0xc002ce2c

    c0044af8:	10400007 	beqz	v0,0xc0044b18
    c0044afc:	001e1902 	srl	v1,s8,0x4
    c0044b00:	dfa500b8 	ld	a1,184(sp)
    c0044b04:	3c02fff8 	lui	v0,0xfff8
    c0044b08:	0222102d 	daddu	v0,s1,v0

    c00457dc:	ffb00050 	sd	s0,80(sp)
    c00457e0:	00a08025 	move	s0,a1
    c00457e4:	ffbf0098 	sd	ra,152(sp)
    c00457e8:	ffbe0090 	sd	s8,144(sp)
    c00457ec:	ffb30068 	sd	s3,104(sp)

    c0045c4c:	8fa20024 	lw	v0,36(sp)
    c0045c50:	24030004 	li	v1,4
    c0045c54:	dfbf0098 	ld	ra,152(sp)
    c0045c58:	00601025 	move	v0,v1
    c0045c5c:	dfbe0090 	ld	s8,144(sp)

    c0048098:	01409825 	move	s3,t2
    c004809c:	ffbf00a8 	sd	ra,168(sp)
    c00480a0:	ffb700a0 	sd	s7,160(sp)
    c00480a4:	ffb60098 	sd	s6,152(sp)
    c00480a8:	ffb50090 	sd	s5,144(sp)

    c00480ac:	ffb20078 	sd	s2,120(sp)

    c00480cc:	dfbf00a8 	ld	ra,168(sp)
    c00480d0:	02c01025 	move	v0,s6
    c00480d4:	dfb700a0 	ld	s7,160(sp)
    c00480d8:	dfb60098 	ld	s6,152(sp)
    c00480dc:	dfb50090 	ld	s5,144(sp)

    c00480e0:	dfb40088 	ld	s4,136(sp)

    c00484d4:	01208825 	move	s1,t1
    c00484d8:	ffbf00a8 	sd	ra,168(sp)
    c00484dc:	ffb700a0 	sd	s7,160(sp)
    c00484e0:	ffb60098 	sd	s6,152(sp)
    c00484e4:	ffb50090 	sd	s5,144(sp)

    c00484e8:	ffb30080 	sd	s3,128(sp)

    c0048744:	00021023 	negu	v0,v0
    c0048748:	dfbf00a8 	ld	ra,168(sp)
    c004874c:	dfb700a0 	ld	s7,160(sp)
    c0048750:	dfb60098 	ld	s6,152(sp)
    c0048754:	dfb50090 	ld	s5,144(sp)

    c0048758:	dfb40088 	ld	s4,136(sp)

    c0049638:	3c020006 	lui	v0,0x6
    c004963c:	02021021 	addu	v0,s0,v0
    c0049640:	ac430498 	sw	v1,1176(v0)
    c0049644:	3c170006 	lui	s7,0x6
    c0049648:	00009025 	move	s2,zero

    c0049f24:	24030001 	li	v1,1
    c0049f28:	02021021 	addu	v0,s0,v0
    c0049f2c:	ac430498 	sw	v1,1176(v0)
    c0049f30:	3c12010d 	lui	s2,0x10d
    c0049f34:	0c00b38b 	jal	0xc002ce2c

    c0049fa4:	02021021 	addu	v0,s0,v0
    c0049fa8:	1000ffe1 	b	0xc0049f30
    c0049fac:	ac400498 	sw	zero,1176(v0)
    c0049fb0:	1251000b 	beq	s2,s1,0xc0049fe0
    c0049fb4:	8fa20004 	lw	v0,4(sp)

    c0049fc0:	00821021 	addu	v0,a0,v0
    c0049fc4:	00571021 	addu	v0,v0,s7
    c0049fc8:	8c420498 	lw	v0,1176(v0)
    c0049fcc:	5040fda1 	beqzl	v0,0xc0049654
    c0049fd0:	26520001 	addiu	s2,s2,1

    c0049fd4:	8c640498 	lw	a0,1176(v1)
    c0049fd8:	144400d1 	bne	v0,a0,0xc004a320
    c0049fdc:	2402fffe 	li	v0,-2

    c004a0fc:	24050002 	li	a1,2
    c004a100:	02042021 	addu	a0,s0,a0
    c004a104:	8c840498 	lw	a0,1176(a0)
    c004a108:	14850058 	bne	a0,a1,0xc004a26c
    c004a10c:	24050001 	li	a1,1

    c004a188:	02048021 	addu	s0,s0,a0
    c004a18c:	24040002 	li	a0,2
    c004a190:	8e050498 	lw	a1,1176(s0)
    c004a194:	14a40004 	bne	a1,a0,0xc004a1a8
    c004a198:	7c030847 	dins	v1,zero,0x1,0x1

    c004b6e8:	ffa90088 	sd	t1,136(sp)
    c004b6ec:	ffaa0090 	sd	t2,144(sp)
    c004b6f0:	ffab0098 	sd	t3,152(sp)
    c004b6f4:	0c02fa09 	jal	0xc00be824
    c004b6f8:	afa60040 	sw	a2,64(sp)

~~~

### OEM bootoctlinux command-table references

- String SPI offset: 0x1ce998
- String offset in Stage-3: 0xce998
- Virtual address after 64-byte boot header: 0xc00ce958

~~~text
(No direct pointer reference found; command table may be relocated or encoded.)
~~~

### Candidate command-handler code windows

(No nearby absolute code pointers found.)
## Bootloader strings

~~~text
0x5b928: U-Boot
0x67048: coremask_override
0x6af60: U-Boot 11.0.0.0-71 (Build time: Mar 12 2024 - 10:35:29)
0x6d910: OCTEON %s, Core clock: %lld MHz
0x1ccd54: coremask    - mask of cores to run on.  Anded with coremask_override environment
0x1ce6c2: coremask    - mask of cores to run on.  Anded with coremask_override
0x1ce998: bootoctlinux
0x1ceac5: coremask    - mask of cores to run on.  Anded with coremask_override
0x1d1ef0: U-Boot
0x1e45f0: coremask_override
0x1e5820: coremask=
0x1e9378: U-Boot 11.0.0.0-71 (Build time: Mar 12 2024 - 10:35:56)
0x1e9498: OCTEON %s, Core clock: %lld MHz
0x1e9f48: OCTEON MMC: ERROR: Only native GPIO pins supported
0x1f6710: PA-220
0x1f6778: bootoctlinux $(loadaddr) $(console) mem=$(mem) rootdelay=10 $(rootfs) numcores=4 $(extra) $(pktbuf) wqe=270336,128 mtdparts=$(mtdpar
~~~

## Interpretation limits

- The OEM Stage-3 is a flat binary in this SPI image, not an ELF with symbols.
- Raw disassembly accounts for the 64-byte BOOT header and its entry point; the file VMA is derived from the header entry point.
- Direct load/store matches and command-table pointers are candidates and require code-context validation before attributing them to bootinfo fields.
- No device access or persistent writes are performed.
