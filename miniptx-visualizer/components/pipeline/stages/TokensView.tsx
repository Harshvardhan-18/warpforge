'use client';

import { usePipelineStore } from '@/store/usePipelineStore';
import { motion } from 'framer-motion';

const TYPE_COLORS: Record<string, string> = {
  KEYWORD: 'text-purple-400',
  IDENT: 'text-blue-300',
  BUILTIN: 'text-yellow-400',
  OPERATOR: 'text-nvidia',
  SYMBOL: 'text-gray-400',
  LITERAL: 'text-orange-400',
  EOF: 'text-gray-600',
};

export default function TokensView() {
  const result = usePipelineStore((s) => s.result);
  if (!result) return null;

  const grouped = new Map<number, typeof result.tokens>();
  for (const tok of result.tokens) {
    if (!grouped.has(tok.line)) grouped.set(tok.line, []);
    grouped.get(tok.line)!.push(tok);
  }

  return (
    <div className="p-4 space-y-4">
      <div className="flex items-center justify-between">
        <h2 className="text-sm font-semibold text-foreground">
          Token Stream
          <span className="ml-2 text-muted-foreground font-normal">
            ({result.tokens.length} tokens)
          </span>
        </h2>
        <div className="flex gap-3 text-xs text-muted-foreground">
          {Object.entries(TYPE_COLORS).map(([type, cls]) => (
            <span key={type} className="flex items-center gap-1">
              <span className={`w-2 h-2 rounded-full ${cls.replace('text-', 'bg-')}`} />
              {type}
            </span>
          ))}
        </div>
      </div>

      {Array.from(grouped.entries()).map(([line, tokens]) => (
        <motion.div
          key={line}
          initial={{ opacity: 0, y: 8 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: line * 0.05 }}
          className="flex items-start gap-3"
        >
          <span className="text-xs text-muted-foreground/50 font-mono w-6 text-right flex-shrink-0 pt-1">
            {line}
          </span>
          <div className="flex flex-wrap gap-1.5">
            {tokens.map((tok, i) => (
              <motion.span
                key={`${line}-${i}`}
                whileHover={{ scale: 1.08, y: -2 }}
                className={`inline-flex items-center gap-1 px-2 py-0.5 rounded text-xs font-mono
                  bg-surface2 border border-border-custom cursor-default
                  transition-colors hover:border-nvidia/40
                  ${TYPE_COLORS[tok.type] || 'text-foreground'}`}
                title={`${tok.type} at ${tok.line}:${tok.col}`}
              >
                <span className="text-muted-foreground/40 text-[10px]">
                  {tok.type}
                </span>
                {tok.value && (
                  <span className="font-semibold">{tok.value}</span>
                )}
              </motion.span>
            ))}
          </div>
        </motion.div>
      ))}
    </div>
  );
}
