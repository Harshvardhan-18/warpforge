'use client';

import { usePipelineStore } from '@/store/usePipelineStore';
import CodeEditor from '@/components/editor/CodeEditor';
import PipelineNav from '@/components/pipeline/PipelineNav';
import TokensView from '@/components/pipeline/stages/TokensView';
import ASTView from '@/components/pipeline/stages/ASTView';
import IRView from '@/components/pipeline/stages/IRView';
import IROptView from '@/components/pipeline/stages/IROptView';
import RegAllocView from '@/components/pipeline/stages/RegAllocView';
import PTXView from '@/components/pipeline/stages/PTXView';
import { motion, AnimatePresence } from 'framer-motion';

const STAGE_COMPONENTS = {
  tokens: TokensView,
  ast: ASTView,
  ir: IRView,
  irOpt: IROptView,
  regalloc: RegAllocView,
  ptx: PTXView,
};

export default function Home() {
  const { activeStage, status, result, error } = usePipelineStore();
  const StageComponent = STAGE_COMPONENTS[activeStage];

  return (
    <div className="h-screen flex flex-col">
      {/* Top bar */}
      <header className="flex items-center justify-between px-6 py-3 border-b border-border-custom bg-surface">
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 rounded-lg bg-nvidia flex items-center justify-center">
            <span className="text-black font-bold text-sm">▶</span>
          </div>
          <div>
            <h1 className="text-sm font-bold tracking-tight">
              Warp<span className="text-nvidia">Forge</span>
            </h1>
            <p className="text-[10px] text-muted-foreground">
              GPU Compiler Pipeline Explorer
            </p>
          </div>
        </div>
        <div className="flex items-center gap-4 text-xs text-muted-foreground">
          <a
            href="https://github.com/Harshvardhan-18/warpforge"
            target="_blank"
            rel="noopener noreferrer"
            className="hover:text-nvidia transition-colors"
          >
            GitHub
          </a>
          <span className="text-border-custom">|</span>
          <span>v0.3</span>
        </div>
      </header>

      {/* Main content */}
      <div className="flex-1 flex min-h-0">
        {/* Left panel: Editor */}
        <div className="w-[420px] flex-shrink-0 border-r border-border-custom flex flex-col">
          <CodeEditor />
        </div>

        {/* Right panel: Pipeline output */}
        <div className="flex-1 flex flex-col min-w-0">
          <PipelineNav />

          <div className="flex-1 overflow-auto">
            <AnimatePresence mode="wait">
              {status === 'idle' && (
                <motion.div
                  key="idle"
                  initial={{ opacity: 0 }}
                  animate={{ opacity: 1 }}
                  exit={{ opacity: 0 }}
                  className="flex flex-col items-center justify-center h-full text-center p-8"
                >
                  <div className="w-16 h-16 rounded-2xl bg-nvidia/10 border border-nvidia/20 flex items-center justify-center mb-4">
                    <span className="text-3xl">⚡</span>
                  </div>
                  <h2 className="text-lg font-semibold mb-2">Ready to Compile</h2>
                  <p className="text-sm text-muted-foreground max-w-md">
                    Write or load a <code className="text-nvidia">.ptxc</code> kernel in the editor,
                    then hit <span className="text-nvidia font-semibold">Compile</span> to explore
                    every stage of the compiler pipeline.
                  </p>
                </motion.div>
              )}

              {status === 'compiling' && (
                <motion.div
                  key="compiling"
                  initial={{ opacity: 0 }}
                  animate={{ opacity: 1 }}
                  exit={{ opacity: 0 }}
                  className="flex flex-col items-center justify-center h-full"
                >
                  <motion.div
                    animate={{ rotate: 360 }}
                    transition={{ repeat: Infinity, duration: 1.5, ease: 'linear' }}
                    className="w-12 h-12 rounded-xl border-2 border-nvidia/30 border-t-nvidia mb-4"
                  />
                  <span className="text-sm text-muted-foreground">
                    Running compiler pipeline…
                  </span>
                </motion.div>
              )}

              {status === 'error' && (
                <motion.div
                  key="error"
                  initial={{ opacity: 0, y: 10 }}
                  animate={{ opacity: 1, y: 0 }}
                  exit={{ opacity: 0 }}
                  className="flex flex-col items-center justify-center h-full p-8"
                >
                  <div className="w-16 h-16 rounded-2xl bg-red-500/10 border border-red-500/20 flex items-center justify-center mb-4">
                    <span className="text-3xl">✗</span>
                  </div>
                  <h2 className="text-lg font-semibold text-red-400 mb-2">Compilation Error</h2>
                  <pre className="text-sm text-red-300 bg-red-500/5 border border-red-500/20 rounded-lg p-4 max-w-lg font-mono whitespace-pre-wrap">
                    {error}
                  </pre>
                </motion.div>
              )}

              {status === 'done' && result && (
                <motion.div
                  key={activeStage}
                  initial={{ opacity: 0, x: 20 }}
                  animate={{ opacity: 1, x: 0 }}
                  exit={{ opacity: 0, x: -20 }}
                  transition={{ duration: 0.2 }}
                >
                  <StageComponent />
                </motion.div>
              )}
            </AnimatePresence>
          </div>
        </div>
      </div>
    </div>
  );
}
