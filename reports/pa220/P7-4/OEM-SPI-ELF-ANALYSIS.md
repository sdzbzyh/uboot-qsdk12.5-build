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
- PA-220 FDT relative offset: 0xc9460
- FDT signature inside extracted image: True

### Raw MIPS64 disassembly: bootinfo-related field offsets

- Full disassembly instruction lines: 319728
- Direct load/store candidates at offsets 152/160/176/184: 503

~~~asm
    c0000e34:	00094c38 	dsll	t1,t1,0x10
    c0000e38:	fd090080 	sd	t1,128(t0)
    c0000e3c:	fd000098 	sd	zero,152(t0)
    c0000e40:	3c0ac000 	lui	t2,0xc000
    c0000e44:	254a1440 	addiu	t2,t2,5184

    c0000e5c:	012a482d 	daddu	t1,t1,t2
    c0000e60:	dd2b0000 	ld	t3,0(t1)
    c0000e64:	fd0b0098 	sd	t3,152(t0)
    c0000e68:	dd2b0008 	ld	t3,8(t1)
    c0000e6c:	fd0b0098 	sd	t3,152(t0)

    c0000e70:	dd000080 	ld	zero,128(t0)
    c0000e74:	3c088001 	lui	t0,0x8001

    c000121c:	ff710088 	sd	s1,136(k1)
    c0001220:	ff720090 	sd	s2,144(k1)
    c0001224:	ff730098 	sd	s3,152(k1)
    c0001228:	ff7400a0 	sd	s4,160(k1)
    c000122c:	ff7500a8 	sd	s5,168(k1)

    c0001230:	ff7600b0 	sd	s6,176(k1)

    c0001234:	ff7700b8 	sd	s7,184(k1)
    c0001238:	ff7800c0 	sd	t8,192(k1)

    c000123c:	ff7900c8 	sd	t9,200(k1)

    c0001654:	ff510088 	sd	s1,136(k0)
    c0001658:	ff520090 	sd	s2,144(k0)
    c000165c:	ff530098 	sd	s3,152(k0)
    c0001660:	ff5400a0 	sd	s4,160(k0)
    c0001664:	ff5500a8 	sd	s5,168(k0)

    c0001668:	ff5600b0 	sd	s6,176(k0)

    c000166c:	ff5700b8 	sd	s7,184(k0)
    c0001670:	ff5800c0 	sd	t8,192(k0)

    c0001674:	ff5900c8 	sd	t9,200(k0)

    c000336c:	ffb300a8 	sd	s3,168(sp)
    c0003370:	00809825 	move	s3,a0
    c0003374:	ffb500b8 	sd	s5,184(sp)
    c0003378:	24040003 	li	a0,3
    c000337c:	ffbf00d8 	sd	ra,216(sp)

    c0003388:	ffb700c8 	sd	s7,200(sp)
    c000338c:	ffb600c0 	sd	s6,192(sp)
    c0003390:	ffb400b0 	sd	s4,176(sp)
    c0003394:	ffb200a0 	sd	s2,160(sp)
    c0003398:	ffb10098 	sd	s1,152(sp)

    c000339c:	0c000811 	jal	0xc0002044

    c00033a0:	ffb00090 	sd	s0,144(sp)

    c0003440:	dfb700c8 	ld	s7,200(sp)
    c0003444:	dfb600c0 	ld	s6,192(sp)
    c0003448:	dfb500b8 	ld	s5,184(sp)
    c000344c:	dfb400b0 	ld	s4,176(sp)
    c0003450:	dfb300a8 	ld	s3,168(sp)

    c0003454:	dfb200a0 	ld	s2,160(sp)

    c0003458:	dfb10098 	ld	s1,152(sp)
    c000345c:	dfb00090 	ld	s0,144(sp)

    c0003460:	03e00008 	jr	ra

    c00043dc:	ffa00088 	sd	zero,136(sp)
    c00043e0:	7c024204 	ins	v0,zero,0x8,0x1
    c00043e4:	ffa000a0 	sd	zero,160(sp)
    c00043e8:	7c0239c4 	ins	v0,zero,0x7,0x1
    c00043ec:	ffa000b0 	sd	zero,176(sp)

    c00043f0:	7c023184 	ins	v0,zero,0x6,0x1
    c00043f4:	ffa000b8 	sd	zero,184(sp)

    c00043f8:	7c622944 	ins	v0,v1,0x5,0x1
    c00043fc:	a7a200d8 	sh	v0,216(sp)

    c000440c:	ffa20090 	sd	v0,144(sp)
    c0004410:	24020080 	li	v0,128
    c0004414:	ffa20098 	sd	v0,152(sp)
    c0004418:	2402ffff 	li	v0,-1
    c000441c:	ffa30080 	sd	v1,128(sp)

    c0005010:	ffbf00a8 	sd	ra,168(sp)
    c0005014:	7c930e02 	dext	s3,a0,0x38,0x2
    c0005018:	ffbe00a0 	sd	s8,160(sp)
    c000501c:	ffb70098 	sd	s7,152(sp)
    c0005020:	ffb50088 	sd	s5,136(sp)

    c0005024:	ffb40080 	sd	s4,128(sp)

    c000503c:	2402ffff 	li	v0,-1
    c0005040:	dfbf00a8 	ld	ra,168(sp)
    c0005044:	dfbe00a0 	ld	s8,160(sp)
    c0005048:	dfb70098 	ld	s7,152(sp)
    c000504c:	dfb60090 	ld	s6,144(sp)

    c0005050:	dfb50088 	ld	s5,136(sp)

    c000b1b8:	27bdff30 	addiu	sp,sp,-208
    c000b1bc:	ffbf00c8 	sd	ra,200(sp)
    c000b1c0:	ffb30098 	sd	s3,152(sp)
    c000b1c4:	ffb20090 	sd	s2,144(sp)
    c000b1c8:	3c12010d 	lui	s2,0x10d

    c000b1d0:	ffb00080 	sd	s0,128(sp)
    c000b1d4:	ffbe00c0 	sd	s8,192(sp)
    c000b1d8:	ffb700b8 	sd	s7,184(sp)
    c000b1dc:	ffb600b0 	sd	s6,176(sp)
    c000b1e0:	ffb500a8 	sd	s5,168(sp)

    c000b1e4:	0c0021c2 	jal	0xc0008708

    c000b1e8:	ffb400a0 	sd	s4,160(sp)
    c000b1ec:	36449508 	ori	a0,s2,0x9508
    c000b1f0:	afa20018 	sw	v0,24(sp)

    c000b250:	8fa20010 	lw	v0,16(sp)
    c000b254:	dfbe00c0 	ld	s8,192(sp)
    c000b258:	dfb700b8 	ld	s7,184(sp)
    c000b25c:	dfb600b0 	ld	s6,176(sp)
    c000b260:	dfb500a8 	ld	s5,168(sp)

    c000b264:	dfb400a0 	ld	s4,160(sp)

    c000b268:	dfb30098 	ld	s3,152(sp)
    c000b26c:	dfb20090 	ld	s2,144(sp)

    c000b270:	dfb10088 	ld	s1,136(sp)

    c001a3d0:	ffb00050 	sd	s0,80(sp)
    c001a3d4:	00028202 	srl	s0,v0,0x8
    c001a3d8:	ffbf0098 	sd	ra,152(sp)
    c001a3dc:	7e140880 	ext	s4,s0,0x2,0x2
    c001a3e0:	ffbe0090 	sd	s8,144(sp)

    c001c348:	14401184 	bnez	v0,0xc002095c
    c001c34c:	afa20008 	sw	v0,8(sp)
    c001c350:	dfbf0098 	ld	ra,152(sp)
    c001c354:	02201025 	move	v0,s1
    c001c358:	dfbe0090 	ld	s8,144(sp)

    c001d99c:	0062102b 	sltu	v0,v1,v0
    c001d9a0:	1440fa6c 	bnez	v0,0xc001c354
    c001d9a4:	dfbf0098 	ld	ra,152(sp)
    c001d9a8:	7c03f83b 	rdhwr	v1,$31
    c001d9ac:	64630064 	daddiu	v1,v1,100

    c001da7c:	0052102b 	sltu	v0,v0,s2
    c001da80:	1440099f 	bnez	v0,0xc0020100
    c001da84:	dfbf0098 	ld	ra,152(sp)
    c001da88:	1000fa33 	b	0xc001c358
    c001da8c:	02201025 	move	v0,s1

    c0020b54:	24840300 	addiu	a0,a0,768
    c0020b58:	1440edfe 	bnez	v0,0xc001c354
    c0020b5c:	dfbf0098 	ld	ra,152(sp)
    c0020b60:	3c12010d 	lui	s2,0x10d
    c0020b64:	0c00b38b 	jal	0xc002ce2c

    c0020b68:	26440308 	addiu	a0,s2,776
    c0020b6c:	1440edf9 	bnez	v0,0xc001c354
    c0020b70:	dfbf0098 	ld	ra,152(sp)
    c0020b74:	0c00b38b 	jal	0xc002ce2c
    c0020b78:	26440600 	addiu	a0,s2,1536

    c0020b7c:	1440edf5 	bnez	v0,0xc001c354
    c0020b80:	dfbf0098 	ld	ra,152(sp)
    c0020b84:	0c00b38b 	jal	0xc002ce2c
    c0020b88:	36449508 	ori	a0,s2,0x9508

    c0020c00:	00000000 	nop
    c0020c04:	c850edd3 	lwc2	$16,-4653(v0)
    c0020c08:	dfbf0098 	ld	ra,152(sp)
    c0020c0c:	e865edd2 	swc2	$5,-4654(v1)
    c0020c10:	02201025 	move	v0,s1

    c0023dac:	ffb40080 	sd	s4,128(sp)
    c0023db0:	0080a025 	move	s4,a0
    c0023db4:	ffbf0098 	sd	ra,152(sp)
    c0023db8:	ffb60090 	sd	s6,144(sp)
    c0023dbc:	ffb30078 	sd	s3,120(sp)

    c0023dd0:	944200c0 	lhu	v0,192(v0)
    c0023dd4:	1455000d 	bne	v0,s5,0xc0023e0c
    c0023dd8:	dfbf0098 	ld	ra,152(sp)
    c0023ddc:	3c10000c 	lui	s0,0xc
    c0023de0:	361003e8 	ori	s0,s0,0x3e8

    c0023e9c:	02402025 	move	a0,s2
    c0023ea0:	1000ffd0 	b	0xc0023de4
    c0023ea4:	dfbf0098 	ld	ra,152(sp)
    c0023ea8:	3c04010d 	lui	a0,0x10d
    c0023eac:	0c00b38b 	jal	0xc002ce2c

    c002404c:	361003e8 	ori	s0,s0,0x3e8
    c0024050:	0682ff64 	bltzl	s4,0xc0023de4
    c0024054:	dfbf0098 	ld	ra,152(sp)
    c0024058:	1000ff8e 	b	0xc0023e94
    c002405c:	02003025 	move	a2,s0

    c0024060:	0440ff60 	bltz	v0,0xc0023de4
    c0024064:	dfbf0098 	ld	ra,152(sp)
    c0024068:	8e220010 	lw	v0,16(s1)
    c002406c:	54400004 	bnezl	v0,0xc0024080

    c00240e4:	00808825 	move	s1,a0
    c00240e8:	ffbf00a8 	sd	ra,168(sp)
    c00240ec:	ffb700a0 	sd	s7,160(sp)
    c00240f0:	ffb60098 	sd	s6,152(sp)
    c00240f4:	ffb50090 	sd	s5,144(sp)

    c00240f8:	ffb40088 	sd	s4,136(sp)

    c002413c:	02202025 	move	a0,s1
    c0024140:	dfbf00a8 	ld	ra,168(sp)
    c0024144:	dfb700a0 	ld	s7,160(sp)
    c0024148:	dfb60098 	ld	s6,152(sp)
    c002414c:	dfb50090 	ld	s5,144(sp)

    c0024150:	dfb40088 	ld	s4,136(sp)

    c0024fe4:	3c02c015 	lui	v0,0xc015
    c0024fe8:	03e00008 	jr	ra
    c0024fec:	8c427da0 	lw	v0,32160(v0)
    c0024ff0:	14800003 	bnez	a0,0xc0025000
    c0024ff4:	2402ffff 	li	v0,-1

    c0025578:	dfb10018 	ld	s1,24(sp)
    c002557c:	dfb00010 	ld	s0,16(sp)
    c0025580:	ac527da0 	sw	s2,32160(v0)
    c0025584:	dfb20020 	ld	s2,32(sp)
    c0025588:	03e00008 	jr	ra

    c0026420:	ffb10010 	sd	s1,16(sp)
    c0026424:	3c11c015 	lui	s1,0xc015
    c0026428:	8e227d98 	lw	v0,32152(s1)
    c002642c:	ffbf0038 	sd	ra,56(sp)
    c0026430:	ffb50030 	sd	s5,48(sp)

    c0026454:	0c000d4f 	jal	0xc000353c
    c0026458:	dc440120 	ld	a0,288(v0)
    c002645c:	ae227d98 	sw	v0,32152(s1)
    c0026460:	3c10010d 	lui	s0,0x10d
    c0026464:	0c00b38b 	jal	0xc002ce2c

    c0026490:	00408025 	move	s0,v0
    c0026494:	0c008671 	jal	0xc00219c4
    c0026498:	8e247d98 	lw	a0,32152(s1)
    c002649c:	14400009 	bnez	v0,0xc00264c4
    c00264a0:	00408025 	move	s0,v0

    c00264b4:	00408025 	move	s0,v0
    c00264b8:	0c0086e0 	jal	0xc0021b80
    c00264bc:	8e247d98 	lw	a0,32152(s1)
    c00264c0:	00408025 	move	s0,v0
    c00264c4:	24118001 	li	s1,-32767

    c0026620:	0c00a2af 	jal	0xc0028abc
    c0026624:	02402025 	move	a0,s2
    c0026628:	8e247d98 	lw	a0,32152(s1)
    c002662c:	0c008890 	jal	0xc0022240
    c0026630:	00402825 	move	a1,v0

    c00269a0:	ace20000 	sw	v0,0(a3)
    c00269a4:	27bdff40 	addiu	sp,sp,-192
    c00269a8:	ffb50098 	sd	s5,152(sp)
    c00269ac:	3c15c014 	lui	s5,0xc014
    c00269b0:	ffb30088 	sd	s3,136(sp)

    c00269cc:	ffa20018 	sd	v0,24(sp)
    c00269d0:	00029000 	sll	s2,v0,0x0
    c00269d4:	ffbf00b8 	sd	ra,184(sp)
    c00269d8:	00111438 	dsll	v0,s1,0x10
    c00269dc:	ffbe00b0 	sd	s8,176(sp)

    c00269e0:	64420001 	daddiu	v0,v0,1
    c00269e4:	ffb700a8 	sd	s7,168(sp)

    c00269e8:	000216f8 	dsll	v0,v0,0x1b
    c00269ec:	ffb600a0 	sd	s6,160(sp)
    c00269f0:	64420010 	daddiu	v0,v0,16
    c00269f4:	ffb40090 	sd	s4,144(sp)

    c0026fb0:	00002025 	move	a0,zero
    c0026fb4:	0000000f 	sync
    c0026fb8:	dfbf00b8 	ld	ra,184(sp)
    c0026fbc:	0014102b 	sltu	v0,zero,s4
    c0026fc0:	dfbe00b0 	ld	s8,176(sp)

    c0026fc4:	dfb700a8 	ld	s7,168(sp)
    c0026fc8:	dfb600a0 	ld	s6,160(sp)

    c0026fcc:	dfb50098 	ld	s5,152(sp)
    c0026fd0:	dfb40090 	ld	s4,144(sp)

    c0026fd4:	dfb30088 	ld	s3,136(sp)

    c002aefc:	fc550008 	sd	s5,8(v0)
    c002af00:	a45400c0 	sh	s4,192(v0)
    c002af04:	ac5100b8 	sw	s1,184(v0)
    c002af08:	24020001 	li	v0,1
    c002af0c:	dfb50030 	ld	s5,48(sp)

    c002ba34:	00c08025 	move	s0,a2
    c002ba38:	afa40004 	sw	a0,4(sp)
    c002ba3c:	ffbf0098 	sd	ra,152(sp)
    c002ba40:	ffb70088 	sd	s7,136(sp)
    c002ba44:	ffb60080 	sd	s6,128(sp)

    c002ba78:	2402ffff 	li	v0,-1
    c002ba7c:	afa20000 	sw	v0,0(sp)
    c002ba80:	dfbf0098 	ld	ra,152(sp)
    c002ba84:	8fa20000 	lw	v0,0(sp)
    c002ba88:	dfbe0090 	ld	s8,144(sp)

    c002c1b0:	ffb40070 	sd	s4,112(sp)
    c002c1b4:	00e0a025 	move	s4,a3
    c002c1b8:	ffbf0098 	sd	ra,152(sp)
    c002c1bc:	ffbe0090 	sd	s8,144(sp)
    c002c1c0:	ffb70088 	sd	s7,136(sp)

    c002c1f4:	3c04fff0 	lui	a0,0xfff0
    c002c1f8:	2412ffff 	li	s2,-1
    c002c1fc:	dfbf0098 	ld	ra,152(sp)
    c002c200:	02401025 	move	v0,s2
    c002c204:	dfbe0090 	ld	s8,144(sp)

    c002c78c:	fe820000 	sd	v0,0(s4)
    c002c790:	27bdff30 	addiu	sp,sp,-208
    c002c794:	ffb600b0 	sd	s6,176(sp)
    c002c798:	0080b025 	move	s6,a0
    c002c79c:	ffb10088 	sd	s1,136(sp)

    c002c7ac:	ffbf00c8 	sd	ra,200(sp)
    c002c7b0:	ffbe00c0 	sd	s8,192(sp)
    c002c7b4:	ffb700b8 	sd	s7,184(sp)
    c002c7b8:	ffb500a8 	sd	s5,168(sp)
    c002c7bc:	ffb400a0 	sd	s4,160(sp)

    c002c7c0:	ffb30098 	sd	s3,152(sp)
    c002c7c4:	ffb20090 	sd	s2,144(sp)

    c002c7c8:	afa5004c 	sw	a1,76(sp)

    c002c7f8:	8fa20010 	lw	v0,16(sp)
    c002c7fc:	dfbe00c0 	ld	s8,192(sp)
    c002c800:	dfb700b8 	ld	s7,184(sp)
    c002c804:	dfb600b0 	ld	s6,176(sp)
    c002c808:	dfb500a8 	ld	s5,168(sp)

    c002c80c:	dfb400a0 	ld	s4,160(sp)

    c002c810:	dfb30098 	ld	s3,152(sp)
    c002c814:	dfb20090 	ld	s2,144(sp)

    c002c818:	dfb10088 	ld	s1,136(sp)

    c002ebf8:	ffb200c0 	sd	s2,192(sp)
    c002ebfc:	00c09025 	move	s2,a2
    c002ec00:	ffb100b8 	sd	s1,184(sp)
    c002ec04:	00808825 	move	s1,a0
    c002ec08:	ffb000b0 	sd	s0,176(sp)

    c002ec0c:	ffbe00f0 	sd	s8,240(sp)
    c002ec10:	ffb700e8 	sd	s7,232(sp)

    c002ece0:	dfb300c8 	ld	s3,200(sp)
    c002ece4:	dfb200c0 	ld	s2,192(sp)
    c002ece8:	dfb100b8 	ld	s1,184(sp)
    c002ecec:	dfb000b0 	ld	s0,176(sp)
    c002ecf0:	0800b40d 	j	0xc002d034

    c002ecf4:	27bd0100 	addiu	sp,sp,256

    c002ed20:	afa800a4 	sw	t0,164(sp)
    c002ed24:	0c00b38b 	jal	0xc002ce2c
    c002ed28:	afa600a0 	sw	a2,160(sp)
    c002ed2c:	03c01825 	move	v1,s8
    c002ed30:	02a03825 	move	a3,s5

    c002ed34:	8e640004 	lw	a0,4(s3)
    c002ed38:	02c2180a 	movz	v1,s6,v0
    c002ed3c:	8fa600a0 	lw	a2,160(sp)
    c002ed40:	8fa800a4 	lw	t0,164(sp)
    c002ed44:	8fa900a8 	lw	t1,168(sp)

    c002edf8:	dfb300c8 	ld	s3,200(sp)
    c002edfc:	dfb200c0 	ld	s2,192(sp)
    c002ee00:	dfb100b8 	ld	s1,184(sp)
    c002ee04:	dfb000b0 	ld	s0,176(sp)
    c002ee08:	03e00008 	jr	ra

    c002ee0c:	27bd0100 	addiu	sp,sp,256

    c0032bac:	00000000 	nop
    c0032bb0:	7ea20c00 	ext	v0,s5,0x10,0x2
    c0032bb4:	afa200b0 	sw	v0,176(sp)
    c0032bb8:	3c10010d 	lui	s0,0x10d
    c0032bbc:	0c00b38b 	jal	0xc002ce2c

    c0032cf4:	3c06c015 	lui	a2,0xc015
    c0032cf8:	00002025 	move	a0,zero
    c0032cfc:	afa600b8 	sw	a2,184(sp)
    c0032d00:	00001025 	move	v0,zero
    c0032d04:	acc57dd8 	sw	a1,32216(a2)

    c0032d24:	10800003 	beqz	a0,0xc0032d34
    c0032d28:	00b08021 	addu	s0,a1,s0
    c0032d2c:	8fa200b8 	lw	v0,184(sp)
    c0032d30:	ac507dd8 	sw	s0,32216(v0)
    c0032d34:	02778021 	addu	s0,s3,s7

    c0032d48:	8e437de0 	lw	v1,32224(s2)
    c0032d4c:	8e020000 	lw	v0,0(s0)
    c0032d50:	8fa400b0 	lw	a0,176(sp)
    c0032d54:	8e9e7de4 	lw	s8,32228(s4)
    c0032d58:	00431021 	addu	v0,v0,v1

    c0032db4:	24050008 	li	a1,8
    c0032db8:	0c00093b 	jal	0xc00024ec
    c0032dbc:	afa600b8 	sw	a2,184(sp)
    c0032dc0:	8fa600b8 	lw	a2,184(sp)
    c0032dc4:	1440ffc7 	bnez	v0,0xc0032ce4

    c0032dc8:	aec27ddc 	sw	v0,32220(s6)

    c0032f54:	8fa200c0 	lw	v0,192(sp)
    c0032f58:	ac447dd0 	sw	a0,32208(v0)
    c0032f5c:	8fa200b0 	lw	v0,176(sp)
    c0032f60:	24070001 	li	a3,1
    c0032f64:	afa300bc 	sw	v1,188(sp)

    c0032f68:	00021100 	sll	v0,v0,0x4
    c0032f6c:	00511025 	or	v0,v0,s1
    c0032f70:	afa200b0 	sw	v0,176(sp)
    c0032f74:	0c00fb42 	jal	0xc003ed08
    c0032f78:	8fa400b0 	lw	a0,176(sp)

    c0032f7c:	0441000d 	bgez	v0,0xc0032fb4
    c0032f80:	8fa300bc 	lw	v1,188(sp)

    c0032fc8:	afa300bc 	sw	v1,188(sp)
    c0032fcc:	8ec57ddc 	lw	a1,32220(s6)
    c0032fd0:	8fa200b8 	lw	v0,184(sp)
    c0032fd4:	00004825 	move	t1,zero
    c0032fd8:	8fc47dcc 	lw	a0,32204(s8)

    c0033014:	11200004 	beqz	t1,0xc0033028
    c0033018:	00a21021 	addu	v0,a1,v0
    c003301c:	8fa500b8 	lw	a1,184(sp)
    c0033020:	ac647dc8 	sw	a0,32200(v1)
    c0033024:	aca27dd8 	sw	v0,32216(a1)

    c0033060:	8e650000 	lw	a1,0(s3)
    c0033064:	0c00fa69 	jal	0xc003e9a4
    c0033068:	8fa400b0 	lw	a0,176(sp)
    c003306c:	0441ff03 	bgez	v0,0xc0032c7c
    c0033070:	00000000 	nop

    c0033144:	027e9825 	or	s3,s3,s8
    c0033148:	03d7b825 	or	s7,s8,s7
    c003314c:	ffa200b0 	sd	v0,176(sp)
    c0033150:	0014a378 	dsll	s4,s4,0xd
    c0033154:	02d0102a 	slt	v0,s6,s0

    c00331ac:	0c00a336 	jal	0xc0028cd8
    c00331b0:	02a02025 	move	a0,s5
    c00331b4:	dfa500b0 	ld	a1,176(sp)
    c00331b8:	32c300ff 	andi	v1,s6,0xff
    c00331bc:	00402025 	move	a0,v0

    c00331e4:	00002025 	move	a0,zero
    c00331e8:	0c0104b4 	jal	0xc00412d0
    c00331ec:	afa200b8 	sw	v0,184(sp)
    c00331f0:	8fa200b8 	lw	v0,184(sp)
    c00331f4:	24030008 	li	v1,8

    c00331f8:	03a03825 	move	a3,sp

    c0034c5c:	ffb00068 	sd	s0,104(sp)
    c0034c60:	00002825 	move	a1,zero
    c0034c64:	ffbf0098 	sd	ra,152(sp)
    c0034c68:	00808025 	move	s0,a0
    c0034c6c:	ffb50090 	sd	s5,144(sp)

    c0034ecc:	24110019 	li	s1,25
    c0034ed0:	00001025 	move	v0,zero
    c0034ed4:	dfbf0098 	ld	ra,152(sp)
    c0034ed8:	dfb50090 	ld	s5,144(sp)
    c0034edc:	dfb40088 	ld	s4,136(sp)

    c0038fcc:	ffb700d8 	sd	s7,216(sp)
    c0038fd0:	ffb600d0 	sd	s6,208(sp)
    c0038fd4:	ffb300b8 	sd	s3,184(sp)
    c0038fd8:	ffb200b0 	sd	s2,176(sp)
    c0038fdc:	0c00a380 	jal	0xc0028e00

    c0038fe0:	ffb000a0 	sd	s0,160(sp)

    c0038fe4:	02a02025 	move	a0,s5
    c0038fe8:	0c00a40a 	jal	0xc0029028

    c0039064:	ffa20088 	sd	v0,136(sp)
    c0039068:	32220001 	andi	v0,s1,0x1
    c003906c:	afa20098 	sw	v0,152(sp)
    c0039070:	3c02fffc 	lui	v0,0xfffc
    c0039074:	64420009 	daddiu	v0,v0,9

    c0039670:	30460003 	andi	a2,v0,0x3
    c0039674:	65080001 	daddiu	t0,t0,1
    c0039678:	8fa20098 	lw	v0,152(sp)
    c003967c:	00021438 	dsll	v0,v0,0x10
    c0039680:	00c2302d 	daddu	a2,a2,v0

    c0039b50:	dc420000 	ld	v0,0(v0)
    c0039b54:	3c08010d 	lui	t0,0x10d
    c0039b58:	afa80098 	sw	t0,152(sp)
    c0039b5c:	0c00b38b 	jal	0xc002ce2c
    c0039b60:	25040200 	addiu	a0,t0,512

    c0039b64:	10400032 	beqz	v0,0xc0039c30
    c0039b68:	8fa80098 	lw	t0,152(sp)
    c0039b6c:	2402000a 	li	v0,10
    c0039b70:	1682004b 	bne	s4,v0,0xc0039ca0

    c003a0d8:	dfb500c8 	ld	s5,200(sp)
    c003a0dc:	dfb400c0 	ld	s4,192(sp)
    c003a0e0:	dfb300b8 	ld	s3,184(sp)
    c003a0e4:	dfb200b0 	ld	s2,176(sp)
    c003a0e8:	dfb100a8 	ld	s1,168(sp)

    c003a0ec:	dfb000a0 	ld	s0,160(sp)

    c003a0f0:	03e00008 	jr	ra
    c003a0f4:	27bd00f0 	addiu	sp,sp,240

    c003f5c4:	7c650240 	ext	a1,v1,0x9,0x1
    c003f5c8:	8cc20008 	lw	v0,8(a2)
    c003f5cc:	dfa700a0 	ld	a3,160(sp)
    c003f5d0:	ffa80050 	sd	t0,80(sp)
    c003f5d4:	ffa90058 	sd	t1,88(sp)

    c003f5f4:	dfb10088 	ld	s1,136(sp)
    c003f5f8:	7c6402c0 	ext	a0,v1,0xb,0x1
    c003f5fc:	dfb20098 	ld	s2,152(sp)
    c003f600:	00852025 	or	a0,a0,a1
    c003f604:	7c827bc4 	ins	v0,a0,0xf,0x1

    c003f760:	ffaa0090 	sd	t2,144(sp)
    c003f764:	0c018cf3 	jal	0xc00633cc
    c003f768:	ffab0098 	sd	t3,152(sp)
    c003f76c:	0c00b38b 	jal	0xc002ce2c
    c003f770:	36049508 	ori	a0,s0,0x9508

    c003f7a0:	dfaa0090 	ld	t2,144(sp)
    c003f7a4:	0c00fd53 	jal	0xc003f54c
    c003f7a8:	dfab0098 	ld	t3,152(sp)
    c003f7ac:	dfbf0058 	ld	ra,88(sp)
    c003f7b0:	00001025 	move	v0,zero

    c00418e8:	afa90038 	sw	t1,56(sp)
    c00418ec:	28a90180 	slti	t1,a1,384
    c00418f0:	ffbf0098 	sd	ra,152(sp)
    c00418f4:	ffbe0090 	sd	s8,144(sp)
    c00418f8:	ffb70088 	sd	s7,136(sp)

    c0041998:	dfa30048 	ld	v1,72(sp)
    c004199c:	00001025 	move	v0,zero
    c00419a0:	dfbf0098 	ld	ra,152(sp)
    c00419a4:	dfbe0090 	ld	s8,144(sp)
    c00419a8:	dfb70088 	ld	s7,136(sp)

    c0041ab4:	01545025 	or	t2,t2,s4
    c0041ab8:	dd420000 	ld	v0,0(t2)
    c0041abc:	8fab00a0 	lw	t3,160(sp)
    c0041ac0:	316b007f 	andi	t3,t3,0x7f
    c0041ac4:	7d62f607 	dins	v0,t3,0x18,0x7

    c0041ae0:	316b001f 	andi	t3,t3,0x1f
    c0041ae4:	7d626207 	dins	v0,t3,0x8,0x5
    c0041ae8:	8fab00b0 	lw	t3,176(sp)
    c0041aec:	7d623807 	dins	v0,t3,0x0,0x8
    c0041af0:	fd420000 	sd	v0,0(t2)

    c0044158:	36249700 	ori	a0,s1,0x9700
    c004415c:	0002180a 	movz	v1,zero,v0
    c0044160:	afa300b8 	sw	v1,184(sp)
    c0044164:	2403001c 	li	v1,28
    c0044168:	0002180a 	movz	v1,zero,v0

    c0044174:	1040005c 	beqz	v0,0xc00442e8
    c0044178:	24020005 	li	v0,5
    c004417c:	afa200b8 	sw	v0,184(sp)
    c0044180:	24020010 	li	v0,16
    c0044184:	afa200c0 	sw	v0,192(sp)

    c00442a8:	54400153 	bnezl	v0,0xc00447f8
    c00442ac:	02809825 	move	s3,s4
    c00442b0:	8fa200b8 	lw	v0,184(sp)
    c00442b4:	3c16c00e 	lui	s6,0xc00e
    c00442b8:	3c17c00e 	lui	s7,0xc00e

    c00442d4:	26d68788 	addiu	s6,s6,-30840
    c00442d8:	26f784e0 	addiu	s7,s7,-31520
    c00442dc:	afa000b0 	sw	zero,176(sp)
    c00442e0:	10000154 	b	0xc0044834
    c00442e4:	02808825 	move	s1,s4

    c00442f0:	1440ffa2 	bnez	v0,0xc004417c
    c00442f4:	24020005 	li	v0,5
    c00442f8:	8fa200b8 	lw	v0,184(sp)
    c00442fc:	1440ffa3 	bnez	v0,0xc004418c
    c0044300:	24060008 	li	a2,8

    c00447e4:	0017b842 	srl	s7,s7,0x1
    c00447e8:	12e0feb2 	beqz	s7,0xc00442b4
    c00447ec:	8fa200b8 	lw	v0,184(sp)
    c00447f0:	1000feab 	b	0xc00442a0
    c00447f4:	241e0001 	li	s8,1

    c004481c:	a04400a8 	sb	a0,168(v0)
    c0044820:	03d2f021 	addu	s8,s8,s2
    c0044824:	8fa200b0 	lw	v0,176(sp)
    c0044828:	26310005 	addiu	s1,s1,5
    c004482c:	24420001 	addiu	v0,v0,1

    c0044830:	afa200b0 	sw	v0,176(sp)
    c0044834:	0c00b38b 	jal	0xc002ce2c
    c0044838:	36a49800 	ori	a0,s5,0x9800

    c00448f0:	1000ffcc 	b	0xc0044824
    c00448f4:	03d2f021 	addu	s8,s8,s2
    c00448f8:	8fa200b0 	lw	v0,176(sp)
    c00448fc:	0044202b 	sltu	a0,v0,a0
    c0044900:	5080ffd9 	beqzl	a0,0xc0044868

    c0044928:	a23e0001 	sb	s8,1(s1)
    c004492c:	03a21021 	addu	v0,sp,v0
    c0044930:	904400a0 	lbu	a0,160(v0)
    c0044934:	00852021 	addu	a0,a0,a1
    c0044938:	a04400a0 	sb	a0,160(v0)

    c004493c:	24040004 	li	a0,4
    c0044940:	5644ffb0 	bnel	s2,a0,0xc0044804

    c00449bc:	14600081 	bnez	v1,0xc0044bc4
    c00449c0:	02201025 	move	v0,s1
    c00449c4:	ffa500b0 	sd	a1,176(sp)
    c00449c8:	0c00b38b 	jal	0xc002ce2c
    c00449cc:	02c02025 	move	a0,s6

    c00449d0:	10400075 	beqz	v0,0xc0044ba8
    c00449d4:	dfa500b0 	ld	a1,176(sp)
    c00449d8:	02201025 	move	v0,s1
    c00449dc:	24030004 	li	v1,4

    c00449e8:	7c023187 	dins	v0,zero,0x6,0x1
    c00449ec:	fea20000 	sd	v0,0(s5)
    c00449f0:	8fa200b8 	lw	v0,184(sp)
    c00449f4:	26730001 	addiu	s3,s3,1
    c00449f8:	1453ffdf 	bne	v0,s3,0xc0044978

    c0044a84:	00118cf8 	dsll	s1,s1,0x13
    c0044a88:	26d68788 	addiu	s6,s6,-30840
    c0044a8c:	afa200b0 	sw	v0,176(sp)
    c0044a90:	26f78550 	addiu	s7,s7,-31408
    c0044a94:	3c12010d 	lui	s2,0x10d

    c0044b0c:	10520006 	beq	v0,s2,0xc0044b28
    c0044b10:	00402825 	move	a1,v0
    c0044b14:	8fa200b0 	lw	v0,176(sp)
    c0044b18:	02c02825 	move	a1,s6
    c0044b1c:	0c023325 	jal	0xc008cc94

    c0044b20:	24448528 	addiu	a0,v0,-31448
    c0044b24:	24050fff 	li	a1,4095
    c0044b28:	ffa500b8 	sd	a1,184(sp)
    c0044b2c:	3c04080d 	lui	a0,0x80d
    c0044b30:	0c00b38b 	jal	0xc002ce2c

    c0044b38:	10400007 	beqz	v0,0xc0044b58
    c0044b3c:	001e1902 	srl	v1,s8,0x4
    c0044b40:	dfa500b8 	ld	a1,184(sp)
    c0044b44:	3c02fff8 	lui	v0,0xfff8
    c0044b48:	0222102d 	daddu	v0,s1,v0

    c004581c:	ffb00050 	sd	s0,80(sp)
    c0045820:	00a08025 	move	s0,a1
    c0045824:	ffbf0098 	sd	ra,152(sp)
    c0045828:	ffbe0090 	sd	s8,144(sp)
    c004582c:	ffb30068 	sd	s3,104(sp)

    c0045c8c:	8fa20024 	lw	v0,36(sp)
    c0045c90:	24030004 	li	v1,4
    c0045c94:	dfbf0098 	ld	ra,152(sp)
    c0045c98:	00601025 	move	v0,v1
    c0045c9c:	dfbe0090 	ld	s8,144(sp)

    c00480d8:	01409825 	move	s3,t2
    c00480dc:	ffbf00a8 	sd	ra,168(sp)
    c00480e0:	ffb700a0 	sd	s7,160(sp)
    c00480e4:	ffb60098 	sd	s6,152(sp)
    c00480e8:	ffb50090 	sd	s5,144(sp)

    c00480ec:	ffb20078 	sd	s2,120(sp)

    c004810c:	dfbf00a8 	ld	ra,168(sp)
    c0048110:	02c01025 	move	v0,s6
    c0048114:	dfb700a0 	ld	s7,160(sp)
    c0048118:	dfb60098 	ld	s6,152(sp)
    c004811c:	dfb50090 	ld	s5,144(sp)

    c0048120:	dfb40088 	ld	s4,136(sp)

    c0048514:	01208825 	move	s1,t1
    c0048518:	ffbf00a8 	sd	ra,168(sp)
    c004851c:	ffb700a0 	sd	s7,160(sp)
    c0048520:	ffb60098 	sd	s6,152(sp)
    c0048524:	ffb50090 	sd	s5,144(sp)

    c0048528:	ffb30080 	sd	s3,128(sp)

    c0048784:	00021023 	negu	v0,v0
    c0048788:	dfbf00a8 	ld	ra,168(sp)
    c004878c:	dfb700a0 	ld	s7,160(sp)
    c0048790:	dfb60098 	ld	s6,152(sp)
    c0048794:	dfb50090 	ld	s5,144(sp)

    c0048798:	dfb40088 	ld	s4,136(sp)

    c0049678:	3c020006 	lui	v0,0x6
    c004967c:	02021021 	addu	v0,s0,v0
    c0049680:	ac430498 	sw	v1,1176(v0)
    c0049684:	3c170006 	lui	s7,0x6
    c0049688:	00009025 	move	s2,zero

    c0049f64:	24030001 	li	v1,1
    c0049f68:	02021021 	addu	v0,s0,v0
    c0049f6c:	ac430498 	sw	v1,1176(v0)
    c0049f70:	3c12010d 	lui	s2,0x10d
    c0049f74:	0c00b38b 	jal	0xc002ce2c

    c0049fe4:	02021021 	addu	v0,s0,v0
    c0049fe8:	1000ffe1 	b	0xc0049f70
    c0049fec:	ac400498 	sw	zero,1176(v0)
    c0049ff0:	1251000b 	beq	s2,s1,0xc004a020
    c0049ff4:	8fa20004 	lw	v0,4(sp)

    c004a000:	00821021 	addu	v0,a0,v0
    c004a004:	00571021 	addu	v0,v0,s7
    c004a008:	8c420498 	lw	v0,1176(v0)
    c004a00c:	5040fda1 	beqzl	v0,0xc0049694
    c004a010:	26520001 	addiu	s2,s2,1

    c004a014:	8c640498 	lw	a0,1176(v1)
    c004a018:	144400d1 	bne	v0,a0,0xc004a360
    c004a01c:	2402fffe 	li	v0,-2

    c004a13c:	24050002 	li	a1,2
    c004a140:	02042021 	addu	a0,s0,a0
    c004a144:	8c840498 	lw	a0,1176(a0)
    c004a148:	14850058 	bne	a0,a1,0xc004a2ac
    c004a14c:	24050001 	li	a1,1

    c004a1c8:	02048021 	addu	s0,s0,a0
    c004a1cc:	24040002 	li	a0,2
    c004a1d0:	8e050498 	lw	a1,1176(s0)
    c004a1d4:	14a40004 	bne	a1,a0,0xc004a1e8
    c004a1d8:	7c030847 	dins	v1,zero,0x1,0x1

    c004b728:	ffa90088 	sd	t1,136(sp)
    c004b72c:	ffaa0090 	sd	t2,144(sp)
    c004b730:	ffab0098 	sd	t3,152(sp)
    c004b734:	0c02fa09 	jal	0xc00be824
    c004b738:	afa60040 	sw	a2,64(sp)

~~~

### OEM bootoctlinux command-table references

- String SPI offset: 0x1ce998
- String offset in Stage-3: 0xce998
- Provisional virtual address: 0xc00ce998

~~~text
reference at Stage-3 +0x13f450 (32-bit): +0x13f438=0x00000000 +0x13f43c=0xc00714ac +0x13f440=0xc00ce420 +0x13f444=0xc00ce4b0 +0x13f448=0x00000000 +0x13f44c=0x00000000 +0x13f450=0xc00ce998 +0x13f454=0x00000020 +0x13f458=0x00000000 +0x13f45c=0xc0071b00 +0x13f460=0xc00ce9a8 +0x13f464=0xc00ce9d0 +0x13f468=0x00000000 +0x13f46c=0x00000000 +0x13f470=0xc00d0b18 +0x13f474=0x00000003 +0x13f478=0x00000001
reference at Stage-3 +0x13f44c (64-bit zero-extended): +0x13f434=0x00000020 +0x13f438=0x00000000 +0x13f43c=0xc00714ac +0x13f440=0xc00ce420 +0x13f444=0xc00ce4b0 +0x13f448=0x00000000 +0x13f44c=0x00000000 +0x13f450=0xc00ce998 +0x13f454=0x00000020 +0x13f458=0x00000000 +0x13f45c=0xc0071b00 +0x13f460=0xc00ce9a8 +0x13f464=0xc00ce9d0 +0x13f468=0x00000000 +0x13f46c=0x00000000 +0x13f470=0xc00d0b18 +0x13f474=0x00000003 +0x13f478=0x00000001
~~~

### Candidate command-handler code windows

#### Candidate address 0xc00714ac

~~~asm

/tmp/pa220-oem-stage3.bin:     file format binary


Disassembly of section .data:

00000000c007146c <.data+0x7146c>:
    c007146c:	00105000 	sll	t2,s0,0x0
    c0071470:	12a00006 	beqz	s5,0xc007148c
    c0071474:	00105000 	sll	t2,s0,0x0
    c0071478:	3210ffff 	andi	s0,s0,0xffff
    c007147c:	00105202 	srl	t2,s0,0x8
    c0071480:	72103a32 	.word	0x72103a32
    c0071484:	020a8025 	or	s0,s0,t2
    c0071488:	00105000 	sll	t2,s0,0x0
    c007148c:	a62a0000 	sh	t2,0(s1)
    c0071490:	1000ffcf 	b	0xc00713d0
    c0071494:	0229882d 	daddu	s1,s1,t1
    c0071498:	a22a0000 	sb	t2,0(s1)
    c007149c:	1000ffcc 	b	0xc00713d0
    c00714a0:	0229882d 	daddu	s1,s1,t1
    c00714a4:	03e00008 	jr	ra
    c00714a8:	00000000 	nop
    c00714ac:	27bdfe50 	addiu	sp,sp,-432
    c00714b0:	00002825 	move	a1,zero
    c00714b4:	ffb20170 	sd	s2,368(sp)
    c00714b8:	27a400e8 	addiu	a0,sp,232
    c00714bc:	ffb50188 	sd	s5,392(sp)
    c00714c0:	00c09025 	move	s2,a2
    c00714c4:	24060078 	li	a2,120
    c00714c8:	00e0a825 	move	s5,a3
    c00714cc:	ffbf01a8 	sd	ra,424(sp)
    c00714d0:	ffbe01a0 	sd	s8,416(sp)
    c00714d4:	ffb70198 	sd	s7,408(sp)
    c00714d8:	ffb60190 	sd	s6,400(sp)
    c00714dc:	ffb40180 	sd	s4,384(sp)
    c00714e0:	ffb30178 	sd	s3,376(sp)
    c00714e4:	ffb10168 	sd	s1,360(sp)
    c00714e8:	0c018c95 	jal	0xc0063254
    c00714ec:	ffb00160 	sd	s0,352(sp)
    c00714f0:	24020001 	li	v0,1
    c00714f4:	ffa200e0 	sd	v0,224(sp)
    c00714f8:	2a420002 	slti	v0,s2,2
    c00714fc:	50400006 	beqzl	v0,0xc0071518
    c0071500:	8ea40004 	lw	a0,4(s5)
    c0071504:	3c02c014 	lui	v0,0xc014
    c0071508:	24100001 	li	s0,1
    c007150c:	8c51e308 	lw	s1,-7416(v0)
    c0071510:	10000037 	b	0xc00715f0
    c0071514:	0000a025 	move	s4,zero
    c0071518:	3c13c00f 	lui	s3,0xc00f
    c007151c:	267348b8 	addiu	s3,s3,18616
    c0071520:	24030002 	li	v1,2
    c0071524:	90820000 	lbu	v0,0(a0)
    c0071528:	00531021 	addu	v0,v0,s3
    c007152c:	90420000 	lbu	v0,0(v0)
    c0071530:	1243004e 	beq	s2,v1,0xc007166c
    c0071534:	30420044 	andi	v0,v0,0x44
    c0071538:	1040000f 	beqz	v0,0xc0071578
    c007153c:	24100001 	li	s0,1
    c0071540:	80830001 	lb	v1,1(a0)
    c0071544:	0003102b 	sltu	v0,zero,v1
    c0071548:	00531021 	addu	v0,v0,s3
    c007154c:	90420000 	lbu	v0,0(v0)
    c0071550:	30420044 	andi	v0,v0,0x44
    c0071554:	14400003 	bnez	v0,0xc0071564
    c0071558:	24060010 	li	a2,16
    c007155c:	14600007 	bnez	v1,0xc007157c
    c0071560:	3c02c014 	lui	v0,0xc014
    c0071564:	0c02f943 	jal	0xc00be50c
    c0071568:	00002825 	move	a1,zero
    c007156c:	24100002 	li	s0,2
    c0071570:	14400003 	bnez	v0,0xc0071580
    c0071574:	00408825 	move	s1,v0
    c0071578:	3c02c014 	lui	v0,0xc014
    c007157c:	8c51e308 	lw	s1,-7416(v0)
    c0071580:	00101080 	sll	v0,s0,0x2
    c0071584:	02a21021 	addu	v0,s5,v0
    c0071588:	8c430000 	lw	v1,0(v0)
    c007158c:	90620000 	lbu	v0,0(v1)
    c0071590:	02621021 	addu	v0,s3,v0
    c0071594:	90420000 	lbu	v0,0(v0)
    c0071598:	30420044 	andi	v0,v0,0x44
    c007159c:	10400014 	beqz	v0,0xc00715f0
    c00715a0:	0000a025 	move	s4,zero
    c00715a4:	80620001 	lb	v0,1(v1)
    c00715a8:	304300ff 	andi	v1,v0,0xff
    c00715ac:	02639821 	addu	s3,s3,v1
    c00715b0:	92630000 	lbu	v1,0(s3)
    c00715b4:	30630044 	andi	v1,v1,0x44
    c00715b8:	14600007 	bnez	v1,0xc00715d8
    c00715bc:	2403ffdf 	li	v1,-33
    c00715c0:	24040058 	li	a0,88
    c00715c4:	00431824 	and	v1,v0,v1
    c00715c8:	50640004 	beql	v1,a0,0xc00715dc
    c00715cc:	8ea40008 	lw	a0,8(s5)
    c00715d0:	14400008 	bnez	v0,0xc00715f4
    c00715d4:	00102880 	sll	a1,s0,0x2
    c00715d8:	8ea40008 	lw	a0,8(s5)
    c00715dc:	24060010 	li	a2,16
    c00715e0:	00002825 	move	a1,zero
    c00715e4:	0c02f943 	jal	0xc00be50c
    c00715e8:	26100001 	addiu	s0,s0,1
    c00715ec:	0040a025 	move	s4,v0
    c00715f0:	00102880 	sll	a1,s0,0x2
    c00715f4:	27a70020 	addiu	a3,sp,32
    c00715f8:	24060002 	li	a2,2
    c00715fc:	02a52821 	addu	a1,s5,a1
    c0071600:	0c01a8c6 	jal	0xc006a318
    c0071604:	02502023 	subu	a0,s2,s0
    c0071608:	93a300dc 	lbu	v1,220(sp)
    c007160c:	10600005 	beqz	v1,0xc0071624
    c0071610:	27a400e0 	addiu	a0,sp,224
    c0071614:	02028021 	addu	s0,s0,v0
    c0071618:	00101080 	sll	v0,s0,0x2
    c007161c:	02509023 	subu	s2,s2,s0
    c0071620:	02a2a821 	addu	s5,s5,v0
    c0071624:	24060080 	li	a2,128
    c0071628:	27a50020 	addiu	a1,sp,32
~~~

#### Candidate address 0xc0071b00

~~~asm

/tmp/pa220-oem-stage3.bin:     file format binary


Disassembly of section .data:

00000000c0071ac0 <.data+0x71ac0>:
    c0071ac0:	8c42c228 	lw	v0,-15832(v0)
    c0071ac4:	ae220724 	sw	v0,1828(s1)
    c0071ac8:	3c02c017 	lui	v0,0xc017
    c0071acc:	24426500 	addiu	v0,v0,25856
    c0071ad0:	dc430078 	ld	v1,120(v0)
    c0071ad4:	ae23072c 	sw	v1,1836(s1)
    c0071ad8:	dc430080 	ld	v1,128(v0)
    c0071adc:	dc420088 	ld	v0,136(v0)
    c0071ae0:	ae230730 	sw	v1,1840(s1)
    c0071ae4:	0c019374 	jal	0xc0064dd0
    c0071ae8:	ae220734 	sw	v0,1844(s1)
    c0071aec:	dfbf0118 	ld	ra,280(sp)
    c0071af0:	dfb10110 	ld	s1,272(sp)
    c0071af4:	dfb00108 	ld	s0,264(sp)
    c0071af8:	03e00008 	jr	ra
    c0071afc:	27bd0120 	addiu	sp,sp,288
    c0071b00:	3c02c016 	lui	v0,0xc016
    c0071b04:	27bdfe20 	addiu	sp,sp,-480
    c0071b08:	244290c0 	addiu	v0,v0,-28480
    c0071b0c:	00002825 	move	a1,zero
    c0071b10:	afa2016c 	sw	v0,364(sp)
    c0071b14:	8fa4016c 	lw	a0,364(sp)
    c0071b18:	afa60160 	sw	a2,352(sp)
    c0071b1c:	24060080 	li	a2,128
    c0071b20:	ffbf01d8 	sd	ra,472(sp)
    c0071b24:	ffb601c0 	sd	s6,448(sp)
    c0071b28:	00e0b025 	move	s6,a3
    c0071b2c:	ffbe01d0 	sd	s8,464(sp)
    c0071b30:	ffb701c8 	sd	s7,456(sp)
    c0071b34:	ffb501b8 	sd	s5,440(sp)
    c0071b38:	ffb401b0 	sd	s4,432(sp)
    c0071b3c:	ffb301a8 	sd	s3,424(sp)
    c0071b40:	ffb201a0 	sd	s2,416(sp)
    c0071b44:	ffb10198 	sd	s1,408(sp)
    c0071b48:	0c018c95 	jal	0xc0063254
    c0071b4c:	ffb00190 	sd	s0,400(sp)
    c0071b50:	24060080 	li	a2,128
    c0071b54:	00002825 	move	a1,zero
    c0071b58:	0c018c95 	jal	0xc0063254
    c0071b5c:	27a400e0 	addiu	a0,sp,224
    c0071b60:	8fa20160 	lw	v0,352(sp)
    c0071b64:	28420002 	slti	v0,v0,2
    c0071b68:	14400049 	bnez	v0,0xc0071c90
    c0071b6c:	3c02c00f 	lui	v0,0xc00f
    c0071b70:	8ec40004 	lw	a0,4(s6)
    c0071b74:	244248b8 	addiu	v0,v0,18616
    c0071b78:	90830000 	lbu	v1,0(a0)
    c0071b7c:	00621821 	addu	v1,v1,v0
    c0071b80:	90630000 	lbu	v1,0(v1)
    c0071b84:	30630044 	andi	v1,v1,0x44
    c0071b88:	50600042 	beqzl	v1,0xc0071c94
    c0071b8c:	24110001 	li	s1,1
    c0071b90:	80830001 	lb	v1,1(a0)
    c0071b94:	306500ff 	andi	a1,v1,0xff
    c0071b98:	00a21021 	addu	v0,a1,v0
    c0071b9c:	90420000 	lbu	v0,0(v0)
    c0071ba0:	30420044 	andi	v0,v0,0x44
    c0071ba4:	14400007 	bnez	v0,0xc0071bc4
    c0071ba8:	2402ffdf 	li	v0,-33
    c0071bac:	24050058 	li	a1,88
    c0071bb0:	00621024 	and	v0,v1,v0
    c0071bb4:	10450004 	beq	v0,a1,0xc0071bc8
    c0071bb8:	24060010 	li	a2,16
    c0071bbc:	14600035 	bnez	v1,0xc0071c94
    c0071bc0:	24110001 	li	s1,1
    c0071bc4:	24060010 	li	a2,16
    c0071bc8:	0c02f943 	jal	0xc00be50c
    c0071bcc:	00002825 	move	a1,zero
    c0071bd0:	24110002 	li	s1,2
    c0071bd4:	14400003 	bnez	v0,0xc0071be4
    c0071bd8:	00408025 	move	s0,v0
    c0071bdc:	3c02c014 	lui	v0,0xc014
    c0071be0:	8c50e308 	lw	s0,-7416(v0)
    c0071be4:	8fa20160 	lw	v0,352(sp)
    c0071be8:	00112880 	sll	a1,s1,0x2
    c0071bec:	27a70020 	addiu	a3,sp,32
    c0071bf0:	24060001 	li	a2,1
    c0071bf4:	02c52821 	addu	a1,s6,a1
    c0071bf8:	0c01a8c6 	jal	0xc006a318
    c0071bfc:	00512023 	subu	a0,v0,s1
    c0071c00:	8fa300d0 	lw	v1,208(sp)
    c0071c04:	afa30180 	sw	v1,384(sp)
    c0071c08:	93a300dc 	lbu	v1,220(sp)
    c0071c0c:	10600008 	beqz	v1,0xc0071c30
    c0071c10:	8fa400c4 	lw	a0,196(sp)
    c0071c14:	02228821 	addu	s1,s1,v0
    c0071c18:	8fa20160 	lw	v0,352(sp)
    c0071c1c:	00511023 	subu	v0,v0,s1
    c0071c20:	00118880 	sll	s1,s1,0x2
    c0071c24:	afa20160 	sw	v0,352(sp)
    c0071c28:	02d1b021 	addu	s6,s6,s1
    c0071c2c:	8fa400c4 	lw	a0,196(sp)
    c0071c30:	5080001b 	beqzl	a0,0xc0071ca0
    c0071c34:	0000b825 	move	s7,zero
    c0071c38:	0c000a89 	jal	0xc0002a24
    c0071c3c:	00000000 	nop
    c0071c40:	14400017 	bnez	v0,0xc0071ca0
    c0071c44:	0040b825 	move	s7,v0
    c0071c48:	8fa500c4 	lw	a1,196(sp)
    c0071c4c:	3c04c00e 	lui	a0,0xc00e
    c0071c50:	0c023325 	jal	0xc008cc94
    c0071c54:	24847388 	addiu	a0,a0,29576
    c0071c58:	24110001 	li	s1,1
    c0071c5c:	dfbf01d8 	ld	ra,472(sp)
    c0071c60:	02201025 	move	v0,s1
    c0071c64:	dfbe01d0 	ld	s8,464(sp)
    c0071c68:	dfb701c8 	ld	s7,456(sp)
    c0071c6c:	dfb601c0 	ld	s6,448(sp)
    c0071c70:	dfb501b8 	ld	s5,440(sp)
    c0071c74:	dfb401b0 	ld	s4,432(sp)
    c0071c78:	dfb301a8 	ld	s3,424(sp)
    c0071c7c:	dfb201a0 	ld	s2,416(sp)
~~~

#### Candidate address 0xc00ce420

~~~asm

/tmp/pa220-oem-stage3.bin:     file format binary


Disassembly of section .data:

00000000c00ce3e0 <.data+0xce3e0>:
    c00ce3e0:	62797465 	daddi	t9,s3,29797
    c00ce3e4:	20737761 	addi	s3,v1,30561
    c00ce3e8:	700a6578 	.word	0x700a6578
    c00ce3ec:	616d706c 	daddi	t5,t3,28780
    c00ce3f0:	653a0a20 	daddiu	k0,t1,2592
    c00ce3f4:	206d6436 	addi	t5,v1,25654
    c00ce3f8:	342e6c73 	ori	t6,at,0x6c73
    c00ce3fc:	20307831 	addi	s0,at,30769
    c00ce400:	30663030 	andi	a2,v1,0x3030
    c00ce404:	30303030 	andi	s0,at,0x3030
    c00ce408:	30000000 	andi	zero,zero,0x0
    c00ce40c:	00000000 	nop
    c00ce410:	626f6f74 	daddi	t7,s3,28532
    c00ce414:	6f637465 	ldr	v1,29797(k1)
    c00ce418:	6c660000 	ldr	a2,0(v1)
    c00ce41c:	00000000 	nop
    c00ce420:	426f6f74 	c0	0x6f6f74
    c00ce424:	20612067 	addi	at,v1,8295
    c00ce428:	656e6572 	daddiu	t6,t3,25970
    c00ce42c:	69632045 	ldl	v1,8261(t3)
    c00ce430:	4c462069 	msub.d	$f1,$f2,$f4,$f6
    c00ce434:	6d616765 	ldr	at,26469(t3)
    c00ce438:	20696e20 	addi	t1,v1,28192
    c00ce43c:	6d656d6f 	ldr	a1,28015(t3)
    c00ce440:	72792e20 	.word	0x72792e20
    c00ce444:	4e4f5445 	.word	0x4e4f5445
    c00ce448:	3a205468 	xori	zero,s1,0x5468
    c00ce44c:	69732063 	ldl	s3,8291(t3)
    c00ce450:	6f6d6d61 	ldr	t5,28001(k1)
    c00ce454:	6e642064 	ldr	a0,8292(s3)
    c00ce458:	6f657320 	ldr	a1,29472(k1)
    c00ce45c:	6e6f740a 	ldr	t7,29706(s3)
    c00ce460:	20202020 	addi	zero,at,8224
    c00ce464:	20202020 	addi	zero,at,8224
    c00ce468:	20202020 	addi	zero,at,8224
    c00ce46c:	20207375 	addi	zero,at,29557
    c00ce470:	70706f72 	.word	0x70706f72
    c00ce474:	74207369 	jalx	0xc081cda5
    c00ce478:	6d706c65 	ldr	s0,27749(t3)
    c00ce47c:	20657865 	addi	a1,v1,30821
    c00ce480:	63757469 	daddi	s5,k1,29801
    c00ce484:	76652061 	jalx	0xc9948185
    c00ce488:	70706c69 	.word	0x70706c69
    c00ce48c:	63617469 	daddi	at,k1,29801
    c00ce490:	6f6e732c 	ldr	t6,29484(k1)
    c00ce494:	20757365 	addi	s5,v1,29541
    c00ce498:	20626f6f 	addi	v0,v1,28527
    c00ce49c:	746f6374 	jalx	0xc1bd8dd1
    c00ce4a0:	20666f72 	addi	a2,v1,28530
    c00ce4a4:	2074686f 	addi	s4,v1,26735
    c00ce4a8:	73652e00 	.word	0x73652e00
    c00ce4ac:	00000000 	nop
    c00ce4b0:	656c665f 	daddiu	t4,t3,26207
    c00ce4b4:	61646472 	daddi	a0,t3,25714
    c00ce4b8:	65737320 	daddiu	s3,t3,29472
    c00ce4bc:	5b6d656d 	.word	0x5b6d656d
    c00ce4c0:	5f73697a 	.word	0x5f73697a
    c00ce4c4:	65205b63 	daddiu	zero,t1,23395
    c00ce4c8:	6f72656d 	ldr	s2,25965(k1)
    c00ce4cc:	61736b3d 	daddi	s3,t3,27453
    c00ce4d0:	6d61736b 	ldr	at,29547(t3)
    c00ce4d4:	5f746f5f 	.word	0x5f746f5f
    c00ce4d8:	72756e20 	.word	0x72756e20
    c00ce4dc:	7c206e75 	.word	0x7c206e75
    c00ce4e0:	6d636f72 	ldr	v1,28530(t3)
    c00ce4e4:	65733d63 	daddiu	s3,t3,15715
    c00ce4e8:	6f72655f 	ldr	s2,25951(k1)
    c00ce4ec:	636e745f 	daddi	t6,k1,29791
    c00ce4f0:	746f5f72 	jalx	0xc1bd7dc9
    c00ce4f4:	756e5d20 	jalx	0xc5b97481
    c00ce4f8:	5b736b69 	.word	0x5b736b69
    c00ce4fc:	70636f72 	.word	0x70636f72
    c00ce500:	65733d63 	daddiu	s3,t3,15715
    c00ce504:	6f72655f 	ldr	s2,25951(k1)
    c00ce508:	636e745f 	daddi	t6,k1,29791
    c00ce50c:	746f5f73 	jalx	0xc1bd7dcd
    c00ce510:	6b69705d 	ldl	t1,28765(k1)
    c00ce514:	205b666f 	addi	k1,v0,26223
    c00ce518:	72636562 	.word	0x72636562
    c00ce51c:	6f6f745d 	ldr	t7,29789(k1)
    c00ce520:	205d0a65 	addi	sp,v0,2661
    c00ce524:	6c665f61 	ldr	a2,24417(v1)
    c00ce528:	64647265 	daddiu	a0,v1,29285
    c00ce52c:	7373202d 	.word	0x7373202d
    c00ce530:	20616464 	addi	at,v1,25700
    c00ce534:	72657373 	.word	0x72657373
    c00ce538:	206f6620 	addi	t7,v1,26144
    c00ce53c:	454c4620 	bc1any4f	$fcc3,0xc00dfdc0
    c00ce540:	696d6167 	ldl	t5,24935(t3)
    c00ce544:	6520746f 	daddiu	zero,t1,29807
    c00ce548:	206c6f61 	addi	t4,v1,28513
    c00ce54c:	642e2020 	daddiu	t6,at,8224
    c00ce550:	49662030 	.word	0x49662030
    c00ce554:	2c206465 	sltiu	zero,at,25701
    c00ce558:	6661756c 	daddiu	at,s3,30060
    c00ce55c:	74206c6f 	jalx	0xc081b1bd
    c00ce560:	61642061 	daddi	a0,t3,8289
    c00ce564:	64647265 	daddiu	a0,v1,29285
    c00ce568:	73732069 	.word	0x73732069
    c00ce56c:	730a2020 	clz	a0 or t2,t8
    c00ce570:	20202020 	addi	zero,at,8224
    c00ce574:	20202020 	addi	zero,at,8224
    c00ce578:	20202020 	addi	zero,at,8224
    c00ce57c:	75736564 	jalx	0xc5cd9591
    c00ce580:	2e0a6d65 	sltiu	t2,s0,28005
    c00ce584:	6d5f7369 	ldr	ra,29545(t2)
    c00ce588:	7a652020 	ld.b	$w0,-411(a0)
    c00ce58c:	20202d20 	addi	zero,at,11552
    c00ce590:	616d6f75 	daddi	t5,t3,28533
    c00ce594:	6e74206f 	ldr	s4,8303(s3)
    c00ce598:	66206d65 	daddiu	zero,s1,28005
    c00ce59c:	6d6f7279 	ldr	t7,29305(t3)
~~~

#### Candidate address 0xc00ce4b0

~~~asm

/tmp/pa220-oem-stage3.bin:     file format binary


Disassembly of section .data:

00000000c00ce470 <.data+0xce470>:
    c00ce470:	70706f72 	.word	0x70706f72
    c00ce474:	74207369 	jalx	0xc081cda5
    c00ce478:	6d706c65 	ldr	s0,27749(t3)
    c00ce47c:	20657865 	addi	a1,v1,30821
    c00ce480:	63757469 	daddi	s5,k1,29801
    c00ce484:	76652061 	jalx	0xc9948185
    c00ce488:	70706c69 	.word	0x70706c69
    c00ce48c:	63617469 	daddi	at,k1,29801
    c00ce490:	6f6e732c 	ldr	t6,29484(k1)
    c00ce494:	20757365 	addi	s5,v1,29541
    c00ce498:	20626f6f 	addi	v0,v1,28527
    c00ce49c:	746f6374 	jalx	0xc1bd8dd1
    c00ce4a0:	20666f72 	addi	a2,v1,28530
    c00ce4a4:	2074686f 	addi	s4,v1,26735
    c00ce4a8:	73652e00 	.word	0x73652e00
    c00ce4ac:	00000000 	nop
    c00ce4b0:	656c665f 	daddiu	t4,t3,26207
    c00ce4b4:	61646472 	daddi	a0,t3,25714
    c00ce4b8:	65737320 	daddiu	s3,t3,29472
    c00ce4bc:	5b6d656d 	.word	0x5b6d656d
    c00ce4c0:	5f73697a 	.word	0x5f73697a
    c00ce4c4:	65205b63 	daddiu	zero,t1,23395
    c00ce4c8:	6f72656d 	ldr	s2,25965(k1)
    c00ce4cc:	61736b3d 	daddi	s3,t3,27453
    c00ce4d0:	6d61736b 	ldr	at,29547(t3)
    c00ce4d4:	5f746f5f 	.word	0x5f746f5f
    c00ce4d8:	72756e20 	.word	0x72756e20
    c00ce4dc:	7c206e75 	.word	0x7c206e75
    c00ce4e0:	6d636f72 	ldr	v1,28530(t3)
    c00ce4e4:	65733d63 	daddiu	s3,t3,15715
    c00ce4e8:	6f72655f 	ldr	s2,25951(k1)
    c00ce4ec:	636e745f 	daddi	t6,k1,29791
    c00ce4f0:	746f5f72 	jalx	0xc1bd7dc9
    c00ce4f4:	756e5d20 	jalx	0xc5b97481
    c00ce4f8:	5b736b69 	.word	0x5b736b69
    c00ce4fc:	70636f72 	.word	0x70636f72
    c00ce500:	65733d63 	daddiu	s3,t3,15715
    c00ce504:	6f72655f 	ldr	s2,25951(k1)
    c00ce508:	636e745f 	daddi	t6,k1,29791
    c00ce50c:	746f5f73 	jalx	0xc1bd7dcd
    c00ce510:	6b69705d 	ldl	t1,28765(k1)
    c00ce514:	205b666f 	addi	k1,v0,26223
    c00ce518:	72636562 	.word	0x72636562
    c00ce51c:	6f6f745d 	ldr	t7,29789(k1)
    c00ce520:	205d0a65 	addi	sp,v0,2661
    c00ce524:	6c665f61 	ldr	a2,24417(v1)
    c00ce528:	64647265 	daddiu	a0,v1,29285
    c00ce52c:	7373202d 	.word	0x7373202d
    c00ce530:	20616464 	addi	at,v1,25700
    c00ce534:	72657373 	.word	0x72657373
    c00ce538:	206f6620 	addi	t7,v1,26144
    c00ce53c:	454c4620 	bc1any4f	$fcc3,0xc00dfdc0
    c00ce540:	696d6167 	ldl	t5,24935(t3)
    c00ce544:	6520746f 	daddiu	zero,t1,29807
    c00ce548:	206c6f61 	addi	t4,v1,28513
    c00ce54c:	642e2020 	daddiu	t6,at,8224
    c00ce550:	49662030 	.word	0x49662030
    c00ce554:	2c206465 	sltiu	zero,at,25701
    c00ce558:	6661756c 	daddiu	at,s3,30060
    c00ce55c:	74206c6f 	jalx	0xc081b1bd
    c00ce560:	61642061 	daddi	a0,t3,8289
    c00ce564:	64647265 	daddiu	a0,v1,29285
    c00ce568:	73732069 	.word	0x73732069
    c00ce56c:	730a2020 	clz	a0 or t2,t8
    c00ce570:	20202020 	addi	zero,at,8224
    c00ce574:	20202020 	addi	zero,at,8224
    c00ce578:	20202020 	addi	zero,at,8224
    c00ce57c:	75736564 	jalx	0xc5cd9591
    c00ce580:	2e0a6d65 	sltiu	t2,s0,28005
    c00ce584:	6d5f7369 	ldr	ra,29545(t2)
    c00ce588:	7a652020 	ld.b	$w0,-411(a0)
    c00ce58c:	20202d20 	addi	zero,at,11552
    c00ce590:	616d6f75 	daddi	t5,t3,28533
    c00ce594:	6e74206f 	ldr	s4,8303(s3)
    c00ce598:	66206d65 	daddiu	zero,s1,28005
    c00ce59c:	6d6f7279 	ldr	t7,29305(t3)
    c00ce5a0:	20746f20 	addi	s4,v1,28448
    c00ce5a4:	72657365 	.word	0x72657365
    c00ce5a8:	72766520 	.word	0x72766520
    c00ce5ac:	666f7220 	daddiu	t7,s3,29216
    c00ce5b0:	454c4620 	bc1any4f	$fcc3,0xc00dfe34
    c00ce5b4:	66696c65 	daddiu	t1,s3,27749
    c00ce5b8:	2e202053 	sltiu	zero,s1,8275
    c00ce5bc:	74617274 	jalx	0xc185c9d1
    c00ce5c0:	73206174 	.word	0x73206174
    c00ce5c4:	20454c46 	addi	a1,v0,19526
    c00ce5c8:	0a202020 	j	0xc8808080
    c00ce5cc:	20202020 	addi	zero,at,8224
    c00ce5d0:	20202020 	addi	zero,at,8224
    c00ce5d4:	20202065 	addi	zero,at,8293
    c00ce5d8:	6e747279 	ldr	s4,29305(s3)
    c00ce5dc:	20706f69 	addi	s0,v1,28521
    c00ce5e0:	6e742072 	ldr	s4,8306(s3)
    c00ce5e4:	6f756e64 	ldr	s5,28260(k1)
    c00ce5e8:	65642064 	daddiu	a0,t3,8292
    c00ce5ec:	6f776e20 	ldr	s7,28192(k1)
    c00ce5f0:	746f2031 	jalx	0xc1bc80c5
    c00ce5f4:	204d4279 	addi	t5,v0,17017
    c00ce5f8:	74652061 	jalx	0xc1948185
    c00ce5fc:	6c69676e 	ldr	t1,26478(v1)
    c00ce600:	6d656e74 	ldr	a1,28276(t3)
    c00ce604:	2e0a2020 	sltiu	t2,s0,8224
    c00ce608:	20202020 	addi	zero,at,8224
    c00ce60c:	20202020 	addi	zero,at,8224
    c00ce610:	20202020 	addi	zero,at,8224
    c00ce614:	4e6f206d 	.word	0x4e6f206d
    c00ce618:	656d6f72 	daddiu	t5,t3,28530
    c00ce61c:	7920616c 	.word	0x7920616c
    c00ce620:	6c6f6361 	ldr	t7,25441(v1)
    c00ce624:	74656420 	jalx	0xc1959081
    c00ce628:	6966206e 	ldl	a2,8302(t3)
    c00ce62c:	6f742073 	ldr	s4,8307(k1)
~~~

#### Candidate address 0xc00ce998

~~~asm

/tmp/pa220-oem-stage3.bin:     file format binary


Disassembly of section .data:

00000000c00ce958 <.data+0xce958>:
    c00ce958:	6c6f6164 	ldr	t7,24932(v1)
    c00ce95c:	20746865 	addi	s4,v1,26725
    c00ce960:	20617070 	addi	at,v1,28784
    c00ce964:	6c696361 	ldr	t1,25441(v1)
    c00ce968:	74696f6e 	jalx	0xc1a5bdb9
    c00ce96c:	20737461 	addi	s3,v1,29793
    c00ce970:	7274696e 	.word	0x7274696e
    c00ce974:	67206174 	daddiu	zero,t9,24948
    c00ce978:	20746865 	addi	s4,v1,26725
    c00ce97c:	206e6578 	addi	t6,v1,25976
    c00ce980:	74206176 	jalx	0xc08185d9
    c00ce984:	61696c61 	daddi	t1,t3,27745
    c00ce988:	626c6520 	daddi	t4,s3,25888
    c00ce98c:	636f7265 	daddi	t7,k1,29285
    c00ce990:	2e0a0a00 	sltiu	t2,s0,2560
    c00ce994:	00000000 	nop
    c00ce998:	626f6f74 	daddi	t7,s3,28532
    c00ce99c:	6f63746c 	ldr	v1,29804(k1)
    c00ce9a0:	696e7578 	ldl	t6,30072(t3)
    c00ce9a4:	00000000 	nop
    c00ce9a8:	426f6f74 	c0	0x6f6f74
    c00ce9ac:	2066726f 	addi	a2,v1,29295
    c00ce9b0:	6d206120 	ldr	zero,24864(t1)
    c00ce9b4:	6c696e75 	ldr	t1,28277(v1)
    c00ce9b8:	7820454c 	.word	0x7820454c
    c00ce9bc:	4620696d 	c1	0x20696d
    c00ce9c0:	61676520 	daddi	a3,t3,25888
    c00ce9c4:	696e206d 	ldl	t6,8301(t3)
    c00ce9c8:	656d6f72 	daddiu	t5,t3,28530
    c00ce9cc:	79000000 	ori.b	$w0,$w0,0x0
    c00ce9d0:	656c665f 	daddiu	t4,t3,26207
    c00ce9d4:	61646472 	daddi	a0,t3,25714
    c00ce9d8:	65737320 	daddiu	s3,t3,29472
    c00ce9dc:	5b636f72 	.word	0x5b636f72
    c00ce9e0:	656d6173 	daddiu	t5,t3,24947
    c00ce9e4:	6b3d6d61 	ldl	sp,28001(t9)
    c00ce9e8:	736b5f74 	.word	0x736b5f74
    c00ce9ec:	6f5f7275 	ldr	ra,29301(k0)
    c00ce9f0:	6e207c20 	ldr	zero,31776(s1)
    c00ce9f4:	6e756d63 	ldr	s5,28003(s3)
    c00ce9f8:	6f726573 	ldr	s2,25971(k1)
    c00ce9fc:	3d636f72 	.word	0x3d636f72
    c00cea00:	655f636e 	daddiu	ra,t2,25454
    c00cea04:	745f746f 	jalx	0xc17dd1bd
    c00cea08:	5f72756e 	.word	0x5f72756e
    c00cea0c:	5d205b66 	bgtzl	t1,0xc00e57a8
    c00cea10:	6f726365 	ldr	s2,25445(k1)
    c00cea14:	626f6f74 	daddi	t7,s3,28532
    c00cea18:	5d205b73 	bgtzl	t1,0xc00e57e8
    c00cea1c:	6b697063 	ldl	t1,28771(k1)
    c00cea20:	6f726573 	ldr	s2,25971(k1)
    c00cea24:	3d636f72 	.word	0x3d636f72
    c00cea28:	655f636e 	daddiu	ra,t2,25454
    c00cea2c:	745f746f 	jalx	0xc17dd1bd
    c00cea30:	5f736b69 	.word	0x5f736b69
    c00cea34:	705d205b 	udi11	v0,sp,a0,0x1
    c00cea38:	6e616d65 	ldr	at,28005(s3)
    c00cea3c:	64626c6f 	daddiu	v0,v1,27759
    c00cea40:	636b3d6e 	daddi	t3,k1,15726
    c00cea44:	616d655d 	daddi	t5,t3,25949
    c00cea48:	205b656e 	addi	k1,v0,25966
    c00cea4c:	64626f6f 	daddiu	v0,v1,28527
    c00cea50:	74617267 	jalx	0xc185c99d
    c00cea54:	735d205b 	udi11	k0,sp,a0,0x1
    c00cea58:	6170705f 	daddi	s0,t3,28767
    c00cea5c:	61726773 	daddi	s2,t3,26483
    c00cea60:	202e2e2e 	addi	t6,at,11822
    c00cea64:	5d0a656c 	.word	0x5d0a656c
    c00cea68:	665f6164 	daddiu	ra,s2,24932
    c00cea6c:	64726573 	daddiu	s2,v1,25971
    c00cea70:	73202d20 	.word	0x73202d20
    c00cea74:	61646472 	daddi	a0,t3,25714
    c00cea78:	65737320 	daddiu	s3,t3,29472
    c00cea7c:	6f662045 	ldr	a2,8261(k1)
    c00cea80:	4c462069 	msub.d	$f1,$f2,$f4,$f6
    c00cea84:	6d616765 	ldr	at,26469(t3)
    c00cea88:	20746f20 	addi	s4,v1,28448
    c00cea8c:	6c6f6164 	ldr	t7,24932(v1)
    c00cea90:	2e204966 	sltiu	zero,s1,18790
    c00cea94:	20302c20 	addi	s0,at,11296
    c00cea98:	64656661 	daddiu	a1,v1,26209
    c00cea9c:	756c7420 	jalx	0xc5b1d081
    c00ceaa0:	6c6f6164 	ldr	t7,24932(v1)
    c00ceaa4:	20616464 	addi	at,v1,25700
    c00ceaa8:	72657373 	.word	0x72657373
    c00ceaac:	0a202020 	j	0xc8808080
    c00ceab0:	20202020 	addi	zero,at,8224
    c00ceab4:	20202020 	addi	zero,at,8224
    c00ceab8:	20202069 	addi	zero,at,8297
    c00ceabc:	73202075 	.word	0x73202075
    c00ceac0:	7365642e 	.word	0x7365642e
    c00ceac4:	0a636f72 	j	0xc98dbdc8
    c00ceac8:	656d6173 	daddiu	t5,t3,24947
    c00ceacc:	6b202020 	ldl	zero,8224(t9)
    c00cead0:	202d206d 	addi	t5,at,8301
    c00cead4:	61736b20 	daddi	s3,t3,27424
    c00cead8:	6f662063 	ldr	a2,8291(k1)
    c00ceadc:	6f726573 	ldr	s2,25971(k1)
    c00ceae0:	20746f20 	addi	s4,v1,28448
    c00ceae4:	72756e20 	.word	0x72756e20
    c00ceae8:	6f6e2e20 	ldr	t6,11808(k1)
    c00ceaec:	20416e64 	addi	at,v0,28260
    c00ceaf0:	65642077 	daddiu	a0,t3,8311
    c00ceaf4:	69746820 	ldl	s4,26656(t3)
    c00ceaf8:	636f7265 	daddi	t7,k1,29285
    c00ceafc:	6d61736b 	ldr	at,29547(t3)
    c00ceb00:	5f6f7665 	.word	0x5f6f7665
    c00ceb04:	72726964 	.word	0x72726964
    c00ceb08:	650a2020 	daddiu	t2,t0,8224
    c00ceb0c:	20202020 	addi	zero,at,8224
    c00ceb10:	20202020 	addi	zero,at,8224
    c00ceb14:	20202020 	addi	zero,at,8224
~~~

#### Candidate address 0xc00ce9a8

~~~asm

/tmp/pa220-oem-stage3.bin:     file format binary


Disassembly of section .data:

00000000c00ce968 <.data+0xce968>:
    c00ce968:	74696f6e 	jalx	0xc1a5bdb9
    c00ce96c:	20737461 	addi	s3,v1,29793
    c00ce970:	7274696e 	.word	0x7274696e
    c00ce974:	67206174 	daddiu	zero,t9,24948
    c00ce978:	20746865 	addi	s4,v1,26725
    c00ce97c:	206e6578 	addi	t6,v1,25976
    c00ce980:	74206176 	jalx	0xc08185d9
    c00ce984:	61696c61 	daddi	t1,t3,27745
    c00ce988:	626c6520 	daddi	t4,s3,25888
    c00ce98c:	636f7265 	daddi	t7,k1,29285
    c00ce990:	2e0a0a00 	sltiu	t2,s0,2560
    c00ce994:	00000000 	nop
    c00ce998:	626f6f74 	daddi	t7,s3,28532
    c00ce99c:	6f63746c 	ldr	v1,29804(k1)
    c00ce9a0:	696e7578 	ldl	t6,30072(t3)
    c00ce9a4:	00000000 	nop
    c00ce9a8:	426f6f74 	c0	0x6f6f74
    c00ce9ac:	2066726f 	addi	a2,v1,29295
    c00ce9b0:	6d206120 	ldr	zero,24864(t1)
    c00ce9b4:	6c696e75 	ldr	t1,28277(v1)
    c00ce9b8:	7820454c 	.word	0x7820454c
    c00ce9bc:	4620696d 	c1	0x20696d
    c00ce9c0:	61676520 	daddi	a3,t3,25888
    c00ce9c4:	696e206d 	ldl	t6,8301(t3)
    c00ce9c8:	656d6f72 	daddiu	t5,t3,28530
    c00ce9cc:	79000000 	ori.b	$w0,$w0,0x0
    c00ce9d0:	656c665f 	daddiu	t4,t3,26207
    c00ce9d4:	61646472 	daddi	a0,t3,25714
    c00ce9d8:	65737320 	daddiu	s3,t3,29472
    c00ce9dc:	5b636f72 	.word	0x5b636f72
    c00ce9e0:	656d6173 	daddiu	t5,t3,24947
    c00ce9e4:	6b3d6d61 	ldl	sp,28001(t9)
    c00ce9e8:	736b5f74 	.word	0x736b5f74
    c00ce9ec:	6f5f7275 	ldr	ra,29301(k0)
    c00ce9f0:	6e207c20 	ldr	zero,31776(s1)
    c00ce9f4:	6e756d63 	ldr	s5,28003(s3)
    c00ce9f8:	6f726573 	ldr	s2,25971(k1)
    c00ce9fc:	3d636f72 	.word	0x3d636f72
    c00cea00:	655f636e 	daddiu	ra,t2,25454
    c00cea04:	745f746f 	jalx	0xc17dd1bd
    c00cea08:	5f72756e 	.word	0x5f72756e
    c00cea0c:	5d205b66 	bgtzl	t1,0xc00e57a8
    c00cea10:	6f726365 	ldr	s2,25445(k1)
    c00cea14:	626f6f74 	daddi	t7,s3,28532
    c00cea18:	5d205b73 	bgtzl	t1,0xc00e57e8
    c00cea1c:	6b697063 	ldl	t1,28771(k1)
    c00cea20:	6f726573 	ldr	s2,25971(k1)
    c00cea24:	3d636f72 	.word	0x3d636f72
    c00cea28:	655f636e 	daddiu	ra,t2,25454
    c00cea2c:	745f746f 	jalx	0xc17dd1bd
    c00cea30:	5f736b69 	.word	0x5f736b69
    c00cea34:	705d205b 	udi11	v0,sp,a0,0x1
    c00cea38:	6e616d65 	ldr	at,28005(s3)
    c00cea3c:	64626c6f 	daddiu	v0,v1,27759
    c00cea40:	636b3d6e 	daddi	t3,k1,15726
    c00cea44:	616d655d 	daddi	t5,t3,25949
    c00cea48:	205b656e 	addi	k1,v0,25966
    c00cea4c:	64626f6f 	daddiu	v0,v1,28527
    c00cea50:	74617267 	jalx	0xc185c99d
    c00cea54:	735d205b 	udi11	k0,sp,a0,0x1
    c00cea58:	6170705f 	daddi	s0,t3,28767
    c00cea5c:	61726773 	daddi	s2,t3,26483
    c00cea60:	202e2e2e 	addi	t6,at,11822
    c00cea64:	5d0a656c 	.word	0x5d0a656c
    c00cea68:	665f6164 	daddiu	ra,s2,24932
    c00cea6c:	64726573 	daddiu	s2,v1,25971
    c00cea70:	73202d20 	.word	0x73202d20
    c00cea74:	61646472 	daddi	a0,t3,25714
    c00cea78:	65737320 	daddiu	s3,t3,29472
    c00cea7c:	6f662045 	ldr	a2,8261(k1)
    c00cea80:	4c462069 	msub.d	$f1,$f2,$f4,$f6
    c00cea84:	6d616765 	ldr	at,26469(t3)
    c00cea88:	20746f20 	addi	s4,v1,28448
    c00cea8c:	6c6f6164 	ldr	t7,24932(v1)
    c00cea90:	2e204966 	sltiu	zero,s1,18790
    c00cea94:	20302c20 	addi	s0,at,11296
    c00cea98:	64656661 	daddiu	a1,v1,26209
    c00cea9c:	756c7420 	jalx	0xc5b1d081
    c00ceaa0:	6c6f6164 	ldr	t7,24932(v1)
    c00ceaa4:	20616464 	addi	at,v1,25700
    c00ceaa8:	72657373 	.word	0x72657373
    c00ceaac:	0a202020 	j	0xc8808080
    c00ceab0:	20202020 	addi	zero,at,8224
    c00ceab4:	20202020 	addi	zero,at,8224
    c00ceab8:	20202069 	addi	zero,at,8297
    c00ceabc:	73202075 	.word	0x73202075
    c00ceac0:	7365642e 	.word	0x7365642e
    c00ceac4:	0a636f72 	j	0xc98dbdc8
    c00ceac8:	656d6173 	daddiu	t5,t3,24947
    c00ceacc:	6b202020 	ldl	zero,8224(t9)
    c00cead0:	202d206d 	addi	t5,at,8301
    c00cead4:	61736b20 	daddi	s3,t3,27424
    c00cead8:	6f662063 	ldr	a2,8291(k1)
    c00ceadc:	6f726573 	ldr	s2,25971(k1)
    c00ceae0:	20746f20 	addi	s4,v1,28448
    c00ceae4:	72756e20 	.word	0x72756e20
    c00ceae8:	6f6e2e20 	ldr	t6,11808(k1)
    c00ceaec:	20416e64 	addi	at,v0,28260
    c00ceaf0:	65642077 	daddiu	a0,t3,8311
    c00ceaf4:	69746820 	ldl	s4,26656(t3)
    c00ceaf8:	636f7265 	daddi	t7,k1,29285
    c00ceafc:	6d61736b 	ldr	at,29547(t3)
    c00ceb00:	5f6f7665 	.word	0x5f6f7665
    c00ceb04:	72726964 	.word	0x72726964
    c00ceb08:	650a2020 	daddiu	t2,t0,8224
    c00ceb0c:	20202020 	addi	zero,at,8224
    c00ceb10:	20202020 	addi	zero,at,8224
    c00ceb14:	20202020 	addi	zero,at,8224
    c00ceb18:	656e7669 	daddiu	t6,t3,30313
    c00ceb1c:	726f6e6d 	.word	0x726f6e6d
    c00ceb20:	656e7420 	daddiu	t6,t3,29728
    c00ceb24:	76617269 	jalx	0xc985c9a5
~~~

#### Candidate address 0xc00ce9d0

~~~asm

/tmp/pa220-oem-stage3.bin:     file format binary


Disassembly of section .data:

00000000c00ce990 <.data+0xce990>:
    c00ce990:	2e0a0a00 	sltiu	t2,s0,2560
    c00ce994:	00000000 	nop
    c00ce998:	626f6f74 	daddi	t7,s3,28532
    c00ce99c:	6f63746c 	ldr	v1,29804(k1)
    c00ce9a0:	696e7578 	ldl	t6,30072(t3)
    c00ce9a4:	00000000 	nop
    c00ce9a8:	426f6f74 	c0	0x6f6f74
    c00ce9ac:	2066726f 	addi	a2,v1,29295
    c00ce9b0:	6d206120 	ldr	zero,24864(t1)
    c00ce9b4:	6c696e75 	ldr	t1,28277(v1)
    c00ce9b8:	7820454c 	.word	0x7820454c
    c00ce9bc:	4620696d 	c1	0x20696d
    c00ce9c0:	61676520 	daddi	a3,t3,25888
    c00ce9c4:	696e206d 	ldl	t6,8301(t3)
    c00ce9c8:	656d6f72 	daddiu	t5,t3,28530
    c00ce9cc:	79000000 	ori.b	$w0,$w0,0x0
    c00ce9d0:	656c665f 	daddiu	t4,t3,26207
    c00ce9d4:	61646472 	daddi	a0,t3,25714
    c00ce9d8:	65737320 	daddiu	s3,t3,29472
    c00ce9dc:	5b636f72 	.word	0x5b636f72
    c00ce9e0:	656d6173 	daddiu	t5,t3,24947
    c00ce9e4:	6b3d6d61 	ldl	sp,28001(t9)
    c00ce9e8:	736b5f74 	.word	0x736b5f74
    c00ce9ec:	6f5f7275 	ldr	ra,29301(k0)
    c00ce9f0:	6e207c20 	ldr	zero,31776(s1)
    c00ce9f4:	6e756d63 	ldr	s5,28003(s3)
    c00ce9f8:	6f726573 	ldr	s2,25971(k1)
    c00ce9fc:	3d636f72 	.word	0x3d636f72
    c00cea00:	655f636e 	daddiu	ra,t2,25454
    c00cea04:	745f746f 	jalx	0xc17dd1bd
    c00cea08:	5f72756e 	.word	0x5f72756e
    c00cea0c:	5d205b66 	bgtzl	t1,0xc00e57a8
    c00cea10:	6f726365 	ldr	s2,25445(k1)
    c00cea14:	626f6f74 	daddi	t7,s3,28532
    c00cea18:	5d205b73 	bgtzl	t1,0xc00e57e8
    c00cea1c:	6b697063 	ldl	t1,28771(k1)
    c00cea20:	6f726573 	ldr	s2,25971(k1)
    c00cea24:	3d636f72 	.word	0x3d636f72
    c00cea28:	655f636e 	daddiu	ra,t2,25454
    c00cea2c:	745f746f 	jalx	0xc17dd1bd
    c00cea30:	5f736b69 	.word	0x5f736b69
    c00cea34:	705d205b 	udi11	v0,sp,a0,0x1
    c00cea38:	6e616d65 	ldr	at,28005(s3)
    c00cea3c:	64626c6f 	daddiu	v0,v1,27759
    c00cea40:	636b3d6e 	daddi	t3,k1,15726
    c00cea44:	616d655d 	daddi	t5,t3,25949
    c00cea48:	205b656e 	addi	k1,v0,25966
    c00cea4c:	64626f6f 	daddiu	v0,v1,28527
    c00cea50:	74617267 	jalx	0xc185c99d
    c00cea54:	735d205b 	udi11	k0,sp,a0,0x1
    c00cea58:	6170705f 	daddi	s0,t3,28767
    c00cea5c:	61726773 	daddi	s2,t3,26483
    c00cea60:	202e2e2e 	addi	t6,at,11822
    c00cea64:	5d0a656c 	.word	0x5d0a656c
    c00cea68:	665f6164 	daddiu	ra,s2,24932
    c00cea6c:	64726573 	daddiu	s2,v1,25971
    c00cea70:	73202d20 	.word	0x73202d20
    c00cea74:	61646472 	daddi	a0,t3,25714
    c00cea78:	65737320 	daddiu	s3,t3,29472
    c00cea7c:	6f662045 	ldr	a2,8261(k1)
    c00cea80:	4c462069 	msub.d	$f1,$f2,$f4,$f6
    c00cea84:	6d616765 	ldr	at,26469(t3)
    c00cea88:	20746f20 	addi	s4,v1,28448
    c00cea8c:	6c6f6164 	ldr	t7,24932(v1)
    c00cea90:	2e204966 	sltiu	zero,s1,18790
    c00cea94:	20302c20 	addi	s0,at,11296
    c00cea98:	64656661 	daddiu	a1,v1,26209
    c00cea9c:	756c7420 	jalx	0xc5b1d081
    c00ceaa0:	6c6f6164 	ldr	t7,24932(v1)
    c00ceaa4:	20616464 	addi	at,v1,25700
    c00ceaa8:	72657373 	.word	0x72657373
    c00ceaac:	0a202020 	j	0xc8808080
    c00ceab0:	20202020 	addi	zero,at,8224
    c00ceab4:	20202020 	addi	zero,at,8224
    c00ceab8:	20202069 	addi	zero,at,8297
    c00ceabc:	73202075 	.word	0x73202075
    c00ceac0:	7365642e 	.word	0x7365642e
    c00ceac4:	0a636f72 	j	0xc98dbdc8
    c00ceac8:	656d6173 	daddiu	t5,t3,24947
    c00ceacc:	6b202020 	ldl	zero,8224(t9)
    c00cead0:	202d206d 	addi	t5,at,8301
    c00cead4:	61736b20 	daddi	s3,t3,27424
    c00cead8:	6f662063 	ldr	a2,8291(k1)
    c00ceadc:	6f726573 	ldr	s2,25971(k1)
    c00ceae0:	20746f20 	addi	s4,v1,28448
    c00ceae4:	72756e20 	.word	0x72756e20
    c00ceae8:	6f6e2e20 	ldr	t6,11808(k1)
    c00ceaec:	20416e64 	addi	at,v0,28260
    c00ceaf0:	65642077 	daddiu	a0,t3,8311
    c00ceaf4:	69746820 	ldl	s4,26656(t3)
    c00ceaf8:	636f7265 	daddi	t7,k1,29285
    c00ceafc:	6d61736b 	ldr	at,29547(t3)
    c00ceb00:	5f6f7665 	.word	0x5f6f7665
    c00ceb04:	72726964 	.word	0x72726964
    c00ceb08:	650a2020 	daddiu	t2,t0,8224
    c00ceb0c:	20202020 	addi	zero,at,8224
    c00ceb10:	20202020 	addi	zero,at,8224
    c00ceb14:	20202020 	addi	zero,at,8224
    c00ceb18:	656e7669 	daddiu	t6,t3,30313
    c00ceb1c:	726f6e6d 	.word	0x726f6e6d
    c00ceb20:	656e7420 	daddiu	t6,t3,29728
    c00ceb24:	76617269 	jalx	0xc985c9a5
    c00ceb28:	61626c65 	daddi	v0,t3,27749
    c00ceb2c:	20746f20 	addi	s4,v1,28448
    c00ceb30:	656e7375 	daddiu	t6,t3,29557
    c00ceb34:	7265206f 	.word	0x7265206f
    c00ceb38:	6e6c7920 	ldr	t4,31008(s3)
    c00ceb3c:	776f726b 	jalx	0xcdbdc9ad
    c00ceb40:	696e6720 	ldl	t6,26400(t3)
    c00ceb44:	636f7265 	daddi	t7,k1,29285
    c00ceb48:	73206172 	.word	0x73206172
    c00ceb4c:	65207573 	daddiu	zero,t1,30067
~~~

#### Candidate address 0xc00d0b18

~~~asm

/tmp/pa220-oem-stage3.bin:     file format binary


Disassembly of section .data:

00000000c00d0ad8 <.data+0xd0ad8>:
    c00d0ad8:	74667470 	jalx	0xc199d1c1
    c00d0adc:	626f6f74 	daddi	t7,s3,28532
	...
    c00d0ae8:	626f6f74 	daddi	t7,s3,28532
    c00d0aec:	20696d61 	addi	t1,v1,28001
    c00d0af0:	67652076 	daddiu	a1,k1,8310
    c00d0af4:	6961206e 	ldl	at,8302(t3)
    c00d0af8:	6574776f 	daddiu	s4,t3,30575
    c00d0afc:	726b2075 	.word	0x726b2075
    c00d0b00:	73696e67 	.word	0x73696e67
    c00d0b04:	20544654 	addi	s4,v0,18004
    c00d0b08:	50207072 	beqzl	at,0xc00eccd4
    c00d0b0c:	6f746f63 	ldr	s4,28515(k1)
    c00d0b10:	6f6c0000 	ldr	t4,0(k1)
    c00d0b14:	00000000 	nop
    c00d0b18:	626f6f74 	daddi	t7,s3,28532
    c00d0b1c:	70000000 	madd	zero,zero
    c00d0b20:	626f6f74 	daddi	t7,s3,28532
    c00d0b24:	20696d61 	addi	t1,v1,28001
    c00d0b28:	67652076 	daddiu	a1,k1,8310
    c00d0b2c:	6961206e 	ldl	at,8302(t3)
    c00d0b30:	6574776f 	daddiu	s4,t3,30575
    c00d0b34:	726b2075 	.word	0x726b2075
    c00d0b38:	73696e67 	.word	0x73696e67
    c00d0b3c:	20424f4f 	addi	v0,v0,20303
    c00d0b40:	54502f54 	bnel	v0,s0,0xc00dc894
    c00d0b44:	46545020 	c1	0x545020
    c00d0b48:	70726f74 	.word	0x70726f74
    c00d0b4c:	6f636f6c 	ldr	v1,28524(k1)
	...
    c00d0b58:	72756e00 	.word	0x72756e00
    c00d0b5c:	00000000 	nop
    c00d0b60:	72756e20 	.word	0x72756e20
    c00d0b64:	636f6d6d 	daddi	t7,k1,28013
    c00d0b68:	616e6473 	daddi	t6,t3,25715
    c00d0b6c:	20696e20 	addi	t1,v1,28192
    c00d0b70:	616e2065 	daddi	t6,t3,8293
    c00d0b74:	6e766972 	ldr	s6,26994(s3)
    c00d0b78:	6f6e6d65 	ldr	t6,28005(k1)
    c00d0b7c:	6e742076 	ldr	s4,8310(s3)
    c00d0b80:	61726961 	daddi	s2,t3,26977
    c00d0b84:	626c6500 	daddi	t4,s3,25856
    c00d0b88:	76617220 	jalx	0xc985c881
    c00d0b8c:	5b2e2e2e 	.word	0x5b2e2e2e
    c00d0b90:	5d0a2020 	.word	0x5d0a2020
    c00d0b94:	20202d20 	addi	zero,at,11552
    c00d0b98:	72756e20 	.word	0x72756e20
    c00d0b9c:	74686520 	jalx	0xc1a19481
    c00d0ba0:	636f6d6d 	daddi	t7,k1,28013
    c00d0ba4:	616e6473 	daddi	t6,t3,25715
    c00d0ba8:	20696e20 	addi	t1,v1,28192
    c00d0bac:	74686520 	jalx	0xc1a19481
    c00d0bb0:	656e7669 	daddiu	t6,t3,30313
    c00d0bb4:	726f6e6d 	.word	0x726f6e6d
    c00d0bb8:	656e7420 	daddiu	t6,t3,29728
    c00d0bbc:	76617269 	jalx	0xc985c9a5
    c00d0bc0:	61626c65 	daddi	v0,t3,27749
    c00d0bc4:	28732920 	slti	s3,v1,10528
    c00d0bc8:	27766172 	addiu	s6,k1,24946
    c00d0bcc:	27000000 	addiu	zero,t8,0
    c00d0bd0:	61736b65 	daddi	s3,t3,27493
    c00d0bd4:	6e760000 	ldr	s6,0(s3)
    c00d0bd8:	67657420 	daddiu	a1,k1,29728
    c00d0bdc:	656e7669 	daddiu	t6,t3,30313
    c00d0be0:	726f6e6d 	.word	0x726f6e6d
    c00d0be4:	656e7420 	daddiu	t6,t3,29728
    c00d0be8:	76617269 	jalx	0xc985c9a5
    c00d0bec:	61626c65 	daddi	v0,t3,27749
    c00d0bf0:	73206672 	.word	0x73206672
    c00d0bf4:	6f6d2073 	ldr	t5,8307(k1)
    c00d0bf8:	7464696e 	jalx	0xc191a5b9
    c00d0bfc:	00000000 	nop
    c00d0c00:	6e616d65 	ldr	at,28005(s3)
    c00d0c04:	205b6d65 	addi	k1,v0,28005
    c00d0c08:	73736167 	.word	0x73736167
    c00d0c0c:	655d205b 	daddiu	sp,t2,8283
    c00d0c10:	73697a65 	.word	0x73697a65
    c00d0c14:	5d0a2020 	.word	0x5d0a2020
    c00d0c18:	20202d20 	addi	zero,at,11552
    c00d0c1c:	67657420 	daddiu	a1,k1,29728
    c00d0c20:	656e7669 	daddiu	t6,t3,30313
    c00d0c24:	726f6e6d 	.word	0x726f6e6d
    c00d0c28:	656e7420 	daddiu	t6,t3,29728
    c00d0c2c:	76617269 	jalx	0xc985c9a5
    c00d0c30:	61626c65 	daddi	v0,t3,27749
    c00d0c34:	20276e61 	addi	a3,at,28257
    c00d0c38:	6d652720 	ldr	a1,10016(t3)
    c00d0c3c:	66726f6d 	daddiu	s2,s3,28525
    c00d0c40:	20737464 	addi	s3,v1,29796
    c00d0c44:	696e2028 	ldl	t6,8232(t3)
    c00d0c48:	6d617820 	ldr	at,30752(t3)
    c00d0c4c:	2773697a 	addiu	s3,k1,27002
    c00d0c50:	65272063 	daddiu	a3,t1,8291
    c00d0c54:	68617273 	ldl	at,29299(v1)
    c00d0c58:	29000000 	slti	zero,t0,0
    c00d0c5c:	00000000 	nop
    c00d0c60:	73657420 	.word	0x73657420
    c00d0c64:	656e7669 	daddiu	t6,t3,30313
    c00d0c68:	726f6e6d 	.word	0x726f6e6d
    c00d0c6c:	656e7420 	daddiu	t6,t3,29728
    c00d0c70:	76617269 	jalx	0xc985c9a5
    c00d0c74:	61626c65 	daddi	v0,t3,27749
    c00d0c78:	73000000 	madd	t8,zero
    c00d0c7c:	00000000 	nop
    c00d0c80:	5b2d665d 	.word	0x5b2d665d
    c00d0c84:	206e616d 	addi	t6,v1,24941
    c00d0c88:	65207661 	daddiu	zero,t1,30305
    c00d0c8c:	6c756520 	ldr	s5,25888(v1)
    c00d0c90:	2e2e2e0a 	sltiu	t6,s1,11786
    c00d0c94:	20202020 	addi	zero,at,8224
~~~

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
- Raw disassembly uses the known Stage-3 image range and a provisional U-Boot text base of 0xc0000000; instruction offsets remain useful even if the linked virtual base differs.
- Direct load/store matches and command-table pointers are candidates and require code-context validation before attributing them to bootinfo fields.
- No device access or persistent writes are performed.
