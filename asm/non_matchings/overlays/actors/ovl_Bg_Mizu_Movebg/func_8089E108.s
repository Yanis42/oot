glabel func_8089E108
/* 004D8 8089E108 000670C0 */  sll     $t6, $a2,  3               
/* 004DC 8089E10C 008E7821 */  addu    $t7, $a0, $t6              
/* 004E0 8089E110 8DE80004 */  lw      $t0, 0x0004($t7)           ## 00000004
/* 004E4 8089E114 3C0A8016 */  lui     $t2, %hi(gSegments)
/* 004E8 8089E118 3C0100FF */  lui     $at, 0x00FF                ## $at = 00FF0000
/* 004EC 8089E11C 0008C100 */  sll     $t8, $t0,  4               
/* 004F0 8089E120 0018CF02 */  srl     $t9, $t8, 28               
/* 004F4 8089E124 00194880 */  sll     $t1, $t9,  2               
/* 004F8 8089E128 01495021 */  addu    $t2, $t2, $t1              
/* 004FC 8089E12C 8D4A6FA8 */  lw      $t2, %lo(gSegments)($t2)
/* 00500 8089E130 3421FFFF */  ori     $at, $at, 0xFFFF           ## $at = 00FFFFFF
/* 00504 8089E134 00076880 */  sll     $t5, $a3,  2               
/* 00508 8089E138 01A76823 */  subu    $t5, $t5, $a3              
/* 0050C 8089E13C 01015824 */  and     $t3, $t0, $at              
/* 00510 8089E140 000D6840 */  sll     $t5, $t5,  1               
/* 00514 8089E144 014B6021 */  addu    $t4, $t2, $t3              
/* 00518 8089E148 018D1821 */  addu    $v1, $t4, $t5              
/* 0051C 8089E14C 3C018000 */  lui     $at, 0x8000                ## $at = 80000000
/* 00520 8089E150 00611821 */  addu    $v1, $v1, $at              
/* 00524 8089E154 846E0000 */  lh      $t6, 0x0000($v1)           ## 00000000
/* 00528 8089E158 00001025 */  or      $v0, $zero, $zero          ## $v0 = 00000000
/* 0052C 8089E15C 448E2000 */  mtc1    $t6, $f4                   ## $f4 = 0.00
/* 00530 8089E160 00000000 */  nop
/* 00534 8089E164 468021A0 */  cvt.s.w $f6, $f4                   
/* 00538 8089E168 E4A60000 */  swc1    $f6, 0x0000($a1)           ## 00000000
/* 0053C 8089E16C 846F0002 */  lh      $t7, 0x0002($v1)           ## 00000002
/* 00540 8089E170 448F4000 */  mtc1    $t7, $f8                   ## $f8 = 0.00
/* 00544 8089E174 00000000 */  nop
/* 00548 8089E178 468042A0 */  cvt.s.w $f10, $f8                  
/* 0054C 8089E17C E4AA0004 */  swc1    $f10, 0x0004($a1)          ## 00000004
/* 00550 8089E180 84780004 */  lh      $t8, 0x0004($v1)           ## 00000004
/* 00554 8089E184 44988000 */  mtc1    $t8, $f16                  ## $f16 = 0.00
/* 00558 8089E188 00000000 */  nop
/* 0055C 8089E18C 468084A0 */  cvt.s.w $f18, $f16                 
/* 00560 8089E190 03E00008 */  jr      $ra                        
/* 00564 8089E194 E4B20008 */  swc1    $f18, 0x0008($a1)          ## 00000008