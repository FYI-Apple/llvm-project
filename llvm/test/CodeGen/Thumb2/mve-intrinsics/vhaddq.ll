; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=thumbv8.1m.main -mattr=+mve.fp -verify-machineinstrs -o - %s | FileCheck %s

define arm_aapcs_vfpcc <16 x i8> @test_vhaddq_u8(<16 x i8> %a, <16 x i8> %b) local_unnamed_addr #0 {
; CHECK-LABEL: test_vhaddq_u8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vhadd.s8 q0, q0, q1
; CHECK-NEXT:    bx lr
entry:
  %0 = tail call <16 x i8> @llvm.arm.mve.vhadd.v16i8(<16 x i8> %a, <16 x i8> %b)
  ret <16 x i8> %0
}

declare <16 x i8> @llvm.arm.mve.vhadd.v16i8(<16 x i8>, <16 x i8>) #1

define arm_aapcs_vfpcc <8 x i16> @test_vhaddq_s16(<8 x i16> %a, <8 x i16> %b) local_unnamed_addr #0 {
; CHECK-LABEL: test_vhaddq_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vhadd.s16 q0, q0, q1
; CHECK-NEXT:    bx lr
entry:
  %0 = tail call <8 x i16> @llvm.arm.mve.vhadd.v8i16(<8 x i16> %a, <8 x i16> %b)
  ret <8 x i16> %0
}

declare <8 x i16> @llvm.arm.mve.vhadd.v8i16(<8 x i16>, <8 x i16>) #1

define arm_aapcs_vfpcc <4 x i32> @test_vhaddq_u32(<4 x i32> %a, <4 x i32> %b) local_unnamed_addr #0 {
; CHECK-LABEL: test_vhaddq_u32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vhadd.s32 q0, q0, q1
; CHECK-NEXT:    bx lr
entry:
  %0 = tail call <4 x i32> @llvm.arm.mve.vhadd.v4i32(<4 x i32> %a, <4 x i32> %b)
  ret <4 x i32> %0
}

declare <4 x i32> @llvm.arm.mve.vhadd.v4i32(<4 x i32>, <4 x i32>) #1

define arm_aapcs_vfpcc <16 x i8> @test_vhaddq_m_s8(<16 x i8> %inactive, <16 x i8> %a, <16 x i8> %b, i16 zeroext %p) local_unnamed_addr #0 {
; CHECK-LABEL: test_vhaddq_m_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vhaddt.s8 q0, q1, q2
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <16 x i1> @llvm.arm.mve.pred.i2v.v16i1(i32 %0)
  %2 = tail call <16 x i8> @llvm.arm.mve.hadd.predicated.v16i8.v16i1(<16 x i8> %a, <16 x i8> %b, <16 x i1> %1, <16 x i8> %inactive)
  ret <16 x i8> %2
}

declare <16 x i1> @llvm.arm.mve.pred.i2v.v16i1(i32) #1

declare <16 x i8> @llvm.arm.mve.hadd.predicated.v16i8.v16i1(<16 x i8>, <16 x i8>, <16 x i1>, <16 x i8>) #1

define arm_aapcs_vfpcc <8 x i16> @test_vhaddq_m_u16(<8 x i16> %inactive, <8 x i16> %a, <8 x i16> %b, i16 zeroext %p) local_unnamed_addr #0 {
; CHECK-LABEL: test_vhaddq_m_u16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vhaddt.s16 q0, q1, q2
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %0)
  %2 = tail call <8 x i16> @llvm.arm.mve.hadd.predicated.v8i16.v8i1(<8 x i16> %a, <8 x i16> %b, <8 x i1> %1, <8 x i16> %inactive)
  ret <8 x i16> %2
}

declare <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32) #1

declare <8 x i16> @llvm.arm.mve.hadd.predicated.v8i16.v8i1(<8 x i16>, <8 x i16>, <8 x i1>, <8 x i16>) #1

define arm_aapcs_vfpcc <4 x i32> @test_vhaddq_m_s32(<4 x i32> %inactive, <4 x i32> %a, <4 x i32> %b, i16 zeroext %p) local_unnamed_addr #0 {
; CHECK-LABEL: test_vhaddq_m_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vhaddt.s32 q0, q1, q2
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x i32> @llvm.arm.mve.hadd.predicated.v4i32.v4i1(<4 x i32> %a, <4 x i32> %b, <4 x i1> %1, <4 x i32> %inactive)
  ret <4 x i32> %2
}

declare <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32) #1

declare <4 x i32> @llvm.arm.mve.hadd.predicated.v4i32.v4i1(<4 x i32>, <4 x i32>, <4 x i1>, <4 x i32>) #1
