'use client';

import { usePipelineStore } from '@/store/usePipelineStore';
import { motion } from 'framer-motion';
import type { StageKey } from '@/lib/types';

const STAGES: { key: StageKey; label: string; icon: string }[] = [
  { key: 'tokens', label: 'Tokens', icon: '🔤' },
  { key: 'ast', label: 'AST', icon: '🌳' },
  { key: 'ir', label: 'IR', icon: '⚙️' },
  { key: 'irOpt', label: 'IR (Opt)', icon: '✨' },
  { key: 'regalloc', label: 'RegAlloc', icon: '🎨' },
  { key: 'ptx', label: 'PTX', icon: '📄' },
];

export default function PipelineNav() {
  const { activeStage, setActiveStage, status, result } = usePipelineStore();

  return (
    <div className="flex items-center gap-1 px-4 py-2 bg-surface border-b border-border-custom overflow-x-auto">
      {STAGES.map((stage, i) => {
        const isActive = activeStage === stage.key;
        const isAvailable = status === 'done' && result !== null;

        return (
          <div key={stage.key} className="flex items-center">
            {i > 0 && (
              <svg className="w-4 h-4 text-border-custom mx-0.5 flex-shrink-0" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5l7 7-7 7" />
              </svg>
            )}
            <motion.button
              whileHover={{ scale: 1.03 }}
              whileTap={{ scale: 0.97 }}
              onClick={() => setActiveStage(stage.key)}
              disabled={!isAvailable}
              className={`relative flex items-center gap-1.5 px-3 py-1.5 rounded-md text-xs font-medium
                transition-all duration-200 whitespace-nowrap
                ${isActive
                  ? 'bg-nvidia/15 text-nvidia border border-nvidia/30'
                  : isAvailable
                    ? 'text-muted-foreground hover:text-foreground hover:bg-surface2'
                    : 'text-muted-foreground/30 cursor-not-allowed'
                }`}
            >
              <span>{stage.icon}</span>
              <span>{stage.label}</span>
              {isActive && (
                <motion.div
                  layoutId="activeStageIndicator"
                  className="absolute inset-0 rounded-md border border-nvidia/30 bg-nvidia/10"
                  style={{ zIndex: -1 }}
                  transition={{ type: 'spring', stiffness: 380, damping: 30 }}
                />
              )}
            </motion.button>
          </div>
        );
      })}

      {/* Stats badge */}
      {result && (
        <motion.div
          initial={{ opacity: 0, x: 10 }}
          animate={{ opacity: 1, x: 0 }}
          className="ml-auto flex items-center gap-3 text-xs text-muted-foreground"
        >
          <span className="flex items-center gap-1">
            <span className="w-2 h-2 rounded-full bg-nvidia" />
            {result.stats.instructions} insts
          </span>
          <span>
            s32:{result.stats.registers.s32} f32:{result.stats.registers.f32} u64:{result.stats.registers.u64}
          </span>
        </motion.div>
      )}
    </div>
  );
}
