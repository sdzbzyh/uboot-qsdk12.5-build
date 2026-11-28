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
- Direct load/store matches are candidates and require code-context validation before attributing them to bootinfo fields.
- No device access or persistent writes are performed.
