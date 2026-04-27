'use client';

import { usePipelineStore } from '@/store/usePipelineStore';
import { motion } from 'framer-motion';

const TYPE_COLORS: Record<string, { dot: string; text: string; ring: string }> = {
  s32: { dot: 'bg-blue-400', text: 'text-blue-400', ring: 'ring-blue-400/30' },
  f32: { dot: 'bg-nvidia', text: 'text-nvidia', ring: 'ring-nvidia/30' },
  u64: { dot: 'bg-purple-400', text: 'text-purple-400', ring: 'ring-purple-400/30' },
  pred: { dot: 'bg-orange-400', text: 'text-orange-400', ring: 'ring-orange-400/30' },
};

export default function RegAllocView() {
  const result = usePipelineStore((s) => s.result);
  if (!result) return null;

  const { nodes, edges, declarations, spillCount } = result.regalloc;

  return (
    <div className="p-4 space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <h2 className="text-sm font-semibold text-foreground">
          Register Allocation
          <span className="ml-2 text-muted-foreground font-normal">
            Chaitin-Briggs Graph Coloring
          </span>
        </h2>
        <div className="flex gap-3 text-xs">
          {Object.entries(TYPE_COLORS).map(([type, cls]) => (
            <span key={type} className={`flex items-center gap-1 ${cls.text}`}>
              <span className={`w-2 h-2 rounded-full ${cls.dot}`} />
              .{type}
            </span>
          ))}
        </div>
      </div>

      {/* Register declarations */}
      <div className="flex gap-2 flex-wrap">
        {declarations.map((decl, i) => (
          <motion.span
            key={i}
            initial={{ opacity: 0, scale: 0.9 }}
            animate={{ opacity: 1, scale: 1 }}
            transition={{ delay: i * 0.05 }}
            className="px-3 py-1 rounded-md text-xs font-mono bg-surface2 border border-border-custom"
          >
            {decl}
          </motion.span>
        ))}
        {spillCount > 0 && (
          <span className="px-3 py-1 rounded-md text-xs font-mono bg-red-500/10 border border-red-500/30 text-red-400">
            ⚠ {spillCount} spills
          </span>
        )}
      </div>

      {/* Interference graph as node grid */}
      <div className="space-y-3">
        <h3 className="text-xs font-semibold text-muted-foreground uppercase tracking-wider">
          Interference Graph
        </h3>
        <div className="grid grid-cols-3 sm:grid-cols-4 md:grid-cols-5 gap-3">
          {nodes.map((node, i) => {
            const color = TYPE_COLORS[node.ptxType] || TYPE_COLORS.s32;
            const neighbors = edges
              .filter((e) => e.source === node.id || e.target === node.id)
              .map((e) => (e.source === node.id ? e.target : e.source));

            return (
              <motion.div
                key={node.id}
                initial={{ opacity: 0, y: 10 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ delay: i * 0.04 }}
                whileHover={{ scale: 1.05, y: -3 }}
                className={`relative p-3 rounded-lg border bg-surface2 cursor-default
                  ring-1 ${color.ring} border-border-custom
                  hover:border-nvidia/40 transition-all`}
              >
                <div className="flex items-center justify-between mb-2">
                  <span className={`font-mono text-sm font-bold ${color.text}`}>
                    {node.id}
                  </span>
                  <span className={`w-2 h-2 rounded-full ${color.dot}`} />
                </div>
                <div className="space-y-1 text-[10px]">
                  <div className="flex justify-between text-muted-foreground">
                    <span>reg</span>
                    <span className="font-mono font-bold text-foreground">{node.ptxReg}</span>
                  </div>
                  <div className="flex justify-between text-muted-foreground">
                    <span>type</span>
                    <span className={color.text}>.{node.ptxType}</span>
                  </div>
                  <div className="flex justify-between text-muted-foreground">
                    <span>uses</span>
                    <span className="text-foreground">{node.useCount}</span>
                  </div>
                </div>
                {neighbors.length > 0 && (
                  <div className="mt-2 pt-2 border-t border-border-custom">
                    <span className="text-[10px] text-muted-foreground/60">
                      ↔ {neighbors.join(' ')}
                    </span>
                  </div>
                )}
                {node.spilled && (
                  <div className="absolute -top-1 -right-1 w-4 h-4 rounded-full bg-red-500 flex items-center justify-center">
                    <span className="text-[8px] text-white font-bold">!</span>
                  </div>
                )}
              </motion.div>
            );
          })}
        </div>
      </div>

      {/* Register mapping table */}
      <div className="space-y-3">
        <h3 className="text-xs font-semibold text-muted-foreground uppercase tracking-wider">
          Coloring Result
        </h3>
        <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-2">
          {nodes.map((node, i) => {
            const color = TYPE_COLORS[node.ptxType] || TYPE_COLORS.s32;
            return (
              <motion.div
                key={node.id}
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                transition={{ delay: 0.3 + i * 0.03 }}
                className="flex items-center justify-between px-3 py-1.5 rounded bg-surface border border-border-custom font-mono text-xs"
              >
                <span className="text-muted-foreground">{node.id}</span>
                <span className="text-muted-foreground/40 mx-1">→</span>
                <span className={`font-bold ${color.text}`}>{node.ptxReg}</span>
              </motion.div>
            );
          })}
        </div>
      </div>
    </div>
  );
}
