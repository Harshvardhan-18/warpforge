'use client';

import { usePipelineStore } from '@/store/usePipelineStore';
import { motion } from 'framer-motion';
import type { IRFunction, IRInstruction } from '@/lib/types';

const KIND_BADGE: Record<string, { bg: string; text: string }> = {
  MOV_BUILTIN: { bg: 'bg-yellow-500/15', text: 'text-yellow-400' },
  BINOP: { bg: 'bg-nvidia/15', text: 'text-nvidia' },
  ICMP: { bg: 'bg-blue-400/15', text: 'text-blue-400' },
  LOAD: { bg: 'bg-cyan-400/15', text: 'text-cyan-400' },
  STORE: { bg: 'bg-pink-400/15', text: 'text-pink-400' },
  BRANCH: { bg: 'bg-orange-400/15', text: 'text-orange-400' },
  JUMP: { bg: 'bg-orange-300/15', text: 'text-orange-300' },
  RET: { bg: 'bg-red-400/15', text: 'text-red-400' },
  ASSIGN: { bg: 'bg-purple-400/15', text: 'text-purple-400' },
  PHI: { bg: 'bg-teal-400/15', text: 'text-teal-400' },
  LABEL: { bg: 'bg-gray-400/15', text: 'text-gray-400' },
};

function InstructionRow({ inst, index }: { inst: IRInstruction; index: number }) {
  const badge = KIND_BADGE[inst.kind] || { bg: 'bg-surface2', text: 'text-foreground' };
  const isRemoved = inst.removedByOpt;

  return (
    <motion.div
      initial={{ opacity: 0, x: -5 }}
      animate={{ opacity: isRemoved ? 0.35 : 1, x: 0 }}
      transition={{ delay: index * 0.02 }}
      className={`flex items-center gap-3 px-3 py-1 rounded font-mono text-xs
        hover:bg-surface2/80 transition-colors group
        ${isRemoved ? 'line-through decoration-red-500/50' : ''}`}
    >
      <span className="text-muted-foreground/30 w-5 text-right">{index}</span>
      <span className={`px-1.5 py-0.5 rounded text-[10px] font-bold ${badge.bg} ${badge.text} w-20 text-center`}>
        {inst.kind}
      </span>
      {inst.dst && <span className="text-nvidia">{inst.dst}</span>}
      {inst.dst && <span className="text-muted-foreground">=</span>}
      {inst.op && <span className="text-foreground font-semibold">{inst.op}</span>}
      {inst.lhs && <span className="text-blue-300">{inst.lhs}</span>}
      {inst.rhs && (
        <>
          <span className="text-muted-foreground">,</span>
          <span className="text-blue-300">{inst.rhs}</span>
        </>
      )}
      {inst.builtin && <span className="text-yellow-400">{inst.builtin}</span>}
      {inst.label && <span className="text-orange-300">→ {inst.label}</span>}
      <span className="ml-auto text-muted-foreground/30 opacity-0 group-hover:opacity-100 transition-opacity">
        L{inst.line}
      </span>
      {isRemoved && (
        <span className="text-[10px] text-red-400 bg-red-400/10 px-1.5 rounded">DCE</span>
      )}
    </motion.div>
  );
}

function IRBlockView({ block, blockIndex }: { block: IRFunction['blocks'][0]; blockIndex: number }) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 10 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ delay: blockIndex * 0.1 }}
      className="border border-border-custom rounded-lg overflow-hidden"
    >
      <div className="flex items-center justify-between px-3 py-1.5 bg-surface2 border-b border-border-custom">
        <span className="text-xs font-mono font-bold text-nvidia">{block.label}:</span>
        {block.successors.length > 0 && (
          <span className="text-[10px] text-muted-foreground">
            → {block.successors.join(', ')}
          </span>
        )}
      </div>
      <div className="py-1">
        {block.instructions.map((inst, i) => (
          <InstructionRow key={inst.id} inst={inst} index={i} />
        ))}
      </div>
    </motion.div>
  );
}

export default function IRView({ optimized = false }: { optimized?: boolean }) {
  const result = usePipelineStore((s) => s.result);
  if (!result) return null;

  const ir = optimized ? result.irOpt : result.ir;

  return (
    <div className="p-4 space-y-4">
      <div className="flex items-center justify-between">
        <h2 className="text-sm font-semibold text-foreground">
          {optimized ? 'IR (Optimised)' : 'IR (Pre-Opt)'}
          <span className="ml-2 text-muted-foreground font-normal">
            function @{ir.name}
          </span>
        </h2>
        {optimized && result.stats.optimisationSummary && (
          <div className="flex gap-3 text-xs">
            {result.stats.optimisationSummary.dceRemoved > 0 && (
              <span className="text-red-400">
                DCE: -{result.stats.optimisationSummary.dceRemoved}
              </span>
            )}
            {result.stats.optimisationSummary.cseRemoved > 0 && (
              <span className="text-orange-400">
                CSE: -{result.stats.optimisationSummary.cseRemoved}
              </span>
            )}
            {result.stats.optimisationSummary.constFoldRemoved > 0 && (
              <span className="text-yellow-400">
                ConstFold: -{result.stats.optimisationSummary.constFoldRemoved}
              </span>
            )}
          </div>
        )}
      </div>

      {/* Params */}
      <div className="flex gap-2 flex-wrap">
        {ir.params.map((p) => (
          <span key={p.name} className="px-2 py-0.5 rounded text-xs font-mono bg-surface2 border border-border-custom">
            <span className="text-muted-foreground">{p.type}</span>
            <span className="text-blue-300 ml-1">{p.name}</span>
          </span>
        ))}
      </div>

      {/* Blocks */}
      <div className="space-y-3">
        {ir.blocks.map((block, i) => (
          <IRBlockView key={block.label} block={block} blockIndex={i} />
        ))}
      </div>
    </div>
  );
}
