'use client';

import { usePipelineStore } from '@/store/usePipelineStore';
import { motion } from 'framer-motion';
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter';
import { vscDarkPlus } from 'react-syntax-highlighter/dist/esm/styles/prism';

export default function PTXView() {
  const result = usePipelineStore((s) => s.result);
  if (!result) return null;

  const customStyle = {
    ...vscDarkPlus,
    'pre[class*="language-"]': {
      ...vscDarkPlus['pre[class*="language-"]'],
      background: '#0a0a0f',
      margin: 0,
      padding: '1rem',
      fontSize: '0.8rem',
      lineHeight: '1.6',
    },
    'code[class*="language-"]': {
      ...vscDarkPlus['code[class*="language-"]'],
      background: 'transparent',
    },
  };

  const handleCopy = () => {
    navigator.clipboard.writeText(result.ptx);
  };

  return (
    <div className="p-4 space-y-4">
      <div className="flex items-center justify-between">
        <h2 className="text-sm font-semibold text-foreground">
          PTX Assembly
          <span className="ml-2 text-muted-foreground font-normal">
            .version 7.0 • .target sm_80
          </span>
        </h2>
        <div className="flex items-center gap-3">
          <div className="flex gap-2 text-xs text-muted-foreground">
            <span className="flex items-center gap-1">
              <span className="w-2 h-2 rounded-full bg-nvidia" />
              {result.stats.instructions} instructions
            </span>
            <span>•</span>
            <span>
              s32:{result.stats.registers.s32}
              {' '}f32:{result.stats.registers.f32}
              {' '}u64:{result.stats.registers.u64}
            </span>
          </div>
          <motion.button
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            onClick={handleCopy}
            className="px-3 py-1 rounded text-xs font-medium bg-surface2 border border-border-custom
              text-muted-foreground hover:text-nvidia hover:border-nvidia/40 transition-colors"
          >
            Copy PTX
          </motion.button>
        </div>
      </div>

      <motion.div
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        className="rounded-lg border border-border-custom overflow-hidden"
      >
        <SyntaxHighlighter
          language="c"
          style={customStyle}
          showLineNumbers
          lineNumberStyle={{
            color: '#333355',
            fontSize: '0.7rem',
            paddingRight: '1rem',
            minWidth: '2.5rem',
            textAlign: 'right',
          }}
          wrapLines
        >
          {result.ptx}
        </SyntaxHighlighter>
      </motion.div>
    </div>
  );
}
