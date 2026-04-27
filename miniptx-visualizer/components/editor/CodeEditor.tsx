'use client';

import { usePipelineStore } from '@/store/usePipelineStore';
import { EXAMPLES } from '@/lib/examples';
import { motion } from 'framer-motion';
import { useRef, useEffect } from 'react';

export default function CodeEditor() {
  const { source, setSource, compile, status } = usePipelineStore();
  const textareaRef = useRef<HTMLTextAreaElement>(null);

  useEffect(() => {
    if (textareaRef.current) {
      textareaRef.current.style.height = 'auto';
      textareaRef.current.style.height = textareaRef.current.scrollHeight + 'px';
    }
  }, [source]);

  return (
    <div className="flex flex-col h-full">
      {/* Header bar */}
      <div className="flex items-center gap-3 px-4 py-2 border-b border-border-custom bg-surface">
        <div className="flex gap-1.5">
          <div className="w-3 h-3 rounded-full bg-red-500/80" />
          <div className="w-3 h-3 rounded-full bg-yellow-500/80" />
          <div className="w-3 h-3 rounded-full bg-green-500/80" />
        </div>
        <span className="text-xs text-muted-foreground font-mono">
          editor.ptxc
        </span>
        <select
          className="ml-auto text-xs bg-surface2 border border-border-custom rounded px-2 py-1 text-muted-foreground focus:outline-none focus:border-nvidia max-w-[180px]"
          onChange={(e) => {
            const ex = EXAMPLES.find((x) => x.label === e.target.value);
            if (ex) setSource(ex.source);
          }}
          defaultValue=""
        >
          <option value="" disabled>
            Load example…
          </option>
          {EXAMPLES.map((ex) => (
            <option key={ex.label} value={ex.label}>
              {ex.label}
            </option>
          ))}
        </select>
      </div>

      {/* Editor area with line numbers */}
      <div className="flex-1 overflow-auto bg-bg">
        <div className="flex min-h-full">
          {/* Line numbers */}
          <div className="flex-shrink-0 py-4 px-2 text-right select-none border-r border-border-custom bg-surface/50">
            {source.split('\n').map((_, i) => (
              <div key={i} className="text-xs text-muted-foreground/50 font-mono leading-6 px-1">
                {i + 1}
              </div>
            ))}
          </div>

          {/* Textarea */}
          <textarea
            ref={textareaRef}
            value={source}
            onChange={(e) => setSource(e.target.value)}
            onKeyDown={(e) => {
              if (e.ctrlKey && e.key === 'Enter') {
                e.preventDefault();
                compile();
              }
            }}
            spellCheck={false}
            className="flex-1 py-4 px-4 font-mono text-sm leading-6 bg-transparent
              text-foreground resize-none outline-none placeholder:text-muted-foreground/30
              selection:bg-nvidia/20"
            placeholder="// Enter your .ptxc kernel here…"
          />
        </div>
      </div>

      {/* Footer with compile button */}
      <div className="flex items-center justify-between px-4 py-3 border-t border-border-custom bg-surface">
        <div className="text-xs text-muted-foreground">
          {status === 'idle' && <span>Ctrl+Enter to compile</span>}
          {status === 'compiling' && (
            <span className="text-nvidia animate-pulse">Compiling…</span>
          )}
          {status === 'done' && (
            <span className="text-nvidia">✓ Compiled</span>
          )}
          {status === 'error' && (
            <span className="text-red-400">✕ Error</span>
          )}
        </div>

        <motion.button
          whileHover={{ scale: 1.03 }}
          whileTap={{ scale: 0.95 }}
          onClick={compile}
          disabled={status === 'compiling'}
          className="flex items-center gap-2 px-5 py-2 rounded-lg text-sm font-bold
            bg-nvidia text-black hover:bg-nvdark disabled:opacity-50
            transition-colors duration-150 shadow-lg shadow-nvidia/20"
        >
          {status === 'compiling' ? (
            <>
              <svg className="animate-spin h-4 w-4" viewBox="0 0 24 24">
                <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" fill="none" />
                <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4z" />
              </svg>
              Compiling…
            </>
          ) : (
            <>
              <svg className="h-4 w-4" fill="currentColor" viewBox="0 0 20 20">
                <path d="M6.3 2.841A1.5 1.5 0 004 4.11v11.78a1.5 1.5 0 002.3 1.269l9.344-5.89a1.5 1.5 0 000-2.538L6.3 2.84z" />
              </svg>
              Compile
            </>
          )}
        </motion.button>
      </div>
    </div>
  );
}
