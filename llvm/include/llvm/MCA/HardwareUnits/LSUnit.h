//===------------------------- LSUnit.h --------------------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
///
/// A Load/Store unit class that models load/store queues and that implements
/// a simple weak memory consistency model.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_MCA_LSUNIT_H
#define LLVM_MCA_LSUNIT_H

#include "llvm/ADT/SmallSet.h"
#include "llvm/MC/MCSchedule.h"
#include "llvm/MCA/HardwareUnits/HardwareUnit.h"
#include "llvm/MCA/Instruction.h"

namespace llvm {
namespace mca {

class Scheduler;

/// Abstract base interface for LS (load/store) units in llvm-mca.
class LSUnitBase : public HardwareUnit {
  /// Load queue size.
  ///
  /// A value of zero for this field means that the load queue is unbounded.
  /// Processor models can declare the size of a load queue via tablegen (see
  /// the definition of tablegen class LoadQueue in
  /// llvm/Target/TargetSchedule.td).
  unsigned LQSize;

  /// Load queue size.
  ///
  /// A value of zero for this field means that the store queue is unbounded.
  /// Processor models can declare the size of a store queue via tablegen (see
  /// the definition of tablegen class StoreQueue in
  /// llvm/Target/TargetSchedule.td).
  unsigned SQSize;

  /// True if loads don't alias with stores.
  ///
  /// By default, the LS unit assumes that loads and stores don't alias with
  /// eachother. If this field is set to false, then loads are always assumed to
  /// alias with stores.
  const bool NoAlias;

public:
  LSUnitBase(const MCSchedModel &SM, unsigned LoadQueueSize,
             unsigned StoreQueueSize, bool AssumeNoAlias);

  virtual ~LSUnitBase();

  /// Returns the total number of entries in the load queue.
  unsigned getLoadQueueSize() const { return LQSize; }

  /// Returns the total number of entries in the store queue.
  unsigned getStoreQueueSize() const { return SQSize; }

  bool assumeNoAlias() const { return NoAlias; }

  enum Status {
    LSU_AVAILABLE = 0,
    LSU_LQUEUE_FULL, // Load Queue unavailable
    LSU_SQUEUE_FULL  // Store Queue unavailable
  };

  /// This method checks the availability of the load/store buffers.
  ///
  /// Returns LSU_AVAILABLE if there are enough load/store queue entries to
  /// accomodate instruction IR. By default, LSU_AVAILABLE is returned if IR is
  /// not a memory operation.
  virtual Status isAvailable(const InstRef &IR) const = 0;

  /// Allocates LS resources for instruction IR.
  ///
  /// This method assumes that a previous call to `isAvailable(IR)` succeeded
  /// with a LSUnitBase::Status value of LSU_AVAILABLE.
  virtual void dispatch(const InstRef &IR) = 0;

  /// Check if a peviously dispatched instruction IR is now ready for execution.
  ///
  /// Instruction IR is assumed to be a memory operation. If IR is still waiting
  /// on another memory instruction M, then M is returned to the caller. If IR
  /// depends on more than one memory operations, then this method returns one
  /// of them.
  ///
  /// Derived classes can implement memory consistency rules for simulated
  /// processor within this member function.
  virtual const InstRef &isReady(const InstRef &IR) const = 0;
};

/// A Load/Store Unit implementing a load and store queues.
///
/// This class implements a load queue and a store queue to emulate the
/// out-of-order execution of memory operations.
/// Each load (or store) consumes an entry in the load (or store) queue.
///
/// Rules are:
/// 1) A younger load is allowed to pass an older load only if there are no
///    stores nor barriers in between the two loads.
/// 2) An younger store is not allowed to pass an older store.
/// 3) A younger store is not allowed to pass an older load.
/// 4) A younger load is allowed to pass an older store only if the load does
///    not alias with the store.
///
/// This class optimistically assumes that loads don't alias store operations.
/// Under this assumption, younger loads are always allowed to pass older
/// stores (this would only affects rule 4).
/// Essentially, this class doesn't perform any sort alias analysis to
/// identify aliasing loads and stores.
///
/// To enforce aliasing between loads and stores, flag `AssumeNoAlias` must be
/// set to `false` by the constructor of LSUnit.
///
/// Note that this class doesn't know about the existence of different memory
/// types for memory operations (example: write-through, write-combining, etc.).
/// Derived classes are responsible for implementing that extra knowledge, and
/// provide different sets of rules for loads and stores by overriding method
/// `isReady()`.
/// To emulate a write-combining memory type, rule 2. must be relaxed in a
/// derived class to enable the reordering of non-aliasing store operations.
///
/// No assumptions are made by this class on the size of the store buffer.  This
/// class doesn't know how to identify cases where store-to-load forwarding may
/// occur.
///
/// LSUnit doesn't attempt to predict whether a load or store hits or misses
/// the L1 cache. To be more specific, LSUnit doesn't know anything about
/// cache hierarchy and memory types.
/// It only knows if an instruction "mayLoad" and/or "mayStore". For loads, the
/// scheduling model provides an "optimistic" load-to-use latency (which usually
/// matches the load-to-use latency for when there is a hit in the L1D).
/// Derived classes may expand this knowledge.
///
/// Class MCInstrDesc in LLVM doesn't know about serializing operations, nor
/// memory-barrier like instructions.
/// LSUnit conservatively assumes that an instruction which `mayLoad` and has
/// `unmodeled side effects` behave like a "soft" load-barrier. That means, it
/// serializes loads without forcing a flush of the load queue.
/// Similarly, instructions that both `mayStore` and have `unmodeled side
/// effects` are treated like store barriers. A full memory
/// barrier is a 'mayLoad' and 'mayStore' instruction with unmodeled side
/// effects. This is obviously inaccurate, but this is the best that we can do
/// at the moment.
///
/// Each load/store barrier consumes one entry in the load/store queue. A
/// load/store barrier enforces ordering of loads/stores:
///  - A younger load cannot pass a load barrier.
///  - A younger store cannot pass a store barrier.
///
/// A younger load has to wait for the memory load barrier to execute.
/// A load/store barrier is "executed" when it becomes the oldest entry in
/// the load/store queue(s). That also means, all the older loads/stores have
/// already been executed.
class LSUnit : public LSUnitBase {
  // When a `MayLoad` instruction is dispatched to the schedulers for execution,
  // the LSUnit reserves an entry in the `LoadQueue` for it.
  //
  // LoadQueue keeps track of all the loads that are in-flight. A load
  // instruction is eventually removed from the LoadQueue when it reaches
  // completion stage. That means, a load leaves the queue whe it is 'executed',
  // and its value can be forwarded on the data path to outside units.
  //
  // This class doesn't know about the latency of a load instruction. So, it
  // conservatively/pessimistically assumes that the latency of a load opcode
  // matches the instruction latency.
  //
  // FIXME: In the absence of cache misses (i.e. L1I/L1D/iTLB/dTLB hits/misses),
  // and load/store conflicts, the latency of a load is determined by the depth
  // of the load pipeline. So, we could use field `LoadLatency` in the
  // MCSchedModel to model that latency.
  // Field `LoadLatency` often matches the so-called 'load-to-use' latency from
  // L1D, and it usually already accounts for any extra latency due to data
  // forwarding.
  // When doing throughput analysis, `LoadLatency` is likely to
  // be a better predictor of load latency than instruction latency. This is
  // particularly true when simulating code with temporal/spatial locality of
  // memory accesses.
  // Using `LoadLatency` (instead of the instruction latency) is also expected
  // to improve the load queue allocation for long latency instructions with
  // folded memory operands (See PR39829).
  //
  // FIXME: On some processors, load/store operations are split into multiple
  // uOps. For example, X86 AMD Jaguar natively supports 128-bit data types, but
  // not 256-bit data types. So, a 256-bit load is effectively split into two
  // 128-bit loads, and each split load consumes one 'LoadQueue' entry. For
  // simplicity, this class optimistically assumes that a load instruction only
  // consumes one entry in the LoadQueue.  Similarly, store instructions only
  // consume a single entry in the StoreQueue.
  // In future, we should reassess the quality of this design, and consider
  // alternative approaches that let instructions specify the number of
  // load/store queue entries which they consume at dispatch stage (See
  // PR39830).
  SmallSet<InstRef, 16> LoadQueue;
  SmallSet<InstRef, 16> StoreQueue;

  void assignLQSlot(const InstRef &IR);
  void assignSQSlot(const InstRef &IR);

  // An instruction that both 'mayStore' and 'HasUnmodeledSideEffects' is
  // conservatively treated as a store barrier. It forces older store to be
  // executed before newer stores are issued.
  SmallSet<InstRef, 8> StoreBarriers;

  // An instruction that both 'MayLoad' and 'HasUnmodeledSideEffects' is
  // conservatively treated as a load barrier. It forces older loads to execute
  // before newer loads are issued.
  SmallSet<InstRef, 8> LoadBarriers;

  bool isSQEmpty() const { return StoreQueue.empty(); }
  bool isLQEmpty() const { return LoadQueue.empty(); }
  bool isSQFull() const {
    return getStoreQueueSize() != 0 && StoreQueue.size() == getStoreQueueSize();
  }
  bool isLQFull() const {
    return getLoadQueueSize() != 0 && LoadQueue.size() == getLoadQueueSize();
  }

public:
  LSUnit(const MCSchedModel &SM)
      : LSUnit(SM, /* LQSize */ 0, /* SQSize */ 0, /* NoAlias */ false) {}
  LSUnit(const MCSchedModel &SM, unsigned LQ, unsigned SQ)
      : LSUnit(SM, LQ, SQ, /* NoAlias */ false) {}
  LSUnit(const MCSchedModel &SM, unsigned LQ, unsigned SQ, bool AssumeNoAlias)
      : LSUnitBase(SM, LQ, SQ, AssumeNoAlias) {}

#ifndef NDEBUG
  void dump() const;
#endif

  /// Returns LSU_AVAILABLE if there are enough load/store queue entries to
  /// accomodate instruction IR.
  Status isAvailable(const InstRef &IR) const override;

  /// Allocates LS resources for instruction IR.
  ///
  /// This method assumes that a previous call to `isAvailable(IR)` succeeded
  /// returning LSU_AVAILABLE.
  void dispatch(const InstRef &IR) override;

  /// Check if a peviously dispatched instruction IR is now ready for execution.
  ///
  /// Rules are:
  /// By default, rules are:
  /// 1. A store may not pass a previous store.
  /// 2. A load may not pass a previous store unless flag 'NoAlias' is set.
  /// 3. A load may pass a previous load.
  /// 4. A store may not pass a previous load (regardless of flag 'NoAlias').
  /// 5. A load has to wait until an older load barrier is fully executed.
  /// 6. A store has to wait until an older store barrier is fully executed.
  const InstRef &isReady(const InstRef &IR) const override;

  /// Instruction executed event handler.
  ///
  /// Load and store instructions are tracked by their corresponding queues from
  /// dispatch until "instruction executed" event.
  /// When a load instruction Ld reaches the 'Executed' stage, its value
  /// is propagated to all the dependent users, and the LS unit stops tracking
  /// Ld.
  /// FIXME: For simplicity, we optimistically assume a similar behavior for
  /// store instructions. In practice, store operations don't tend to leave the
  /// store queue until they reach the 'Retired' stage (See PR39830).
  void onInstructionExecuted(const InstRef &IR);
};

} // namespace mca
} // namespace llvm

#endif // LLVM_MCA_LSUNIT_H
