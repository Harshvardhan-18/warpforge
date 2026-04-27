'use client';

import { usePipelineStore } from '@/store/usePipelineStore';
import { motion } from 'framer-motion';
import type { ASTNode } from '@/lib/types';
import { useState } from 'react';

const KIND_COLORS: Record<string, string> = {
  KernelDecl: 'border-purple-500 bg-purple-500/10',
  ParamDecl: 'border-blue-400 bg-blue-400/10',
  BlockStmt: 'border-gray-500 bg-gray-500/5',
  VarDeclStmt: 'border-cyan-400 bg-cyan-400/10',
  IfStmt: 'border-yellow-400 bg-yellow-400/10',
  ForStmt: 'border-orange-400 bg-orange-400/10',
  BinaryExpr: 'border-nvidia bg-nvidia/10',
  AssignExpr: 'border-pink-400 bg-pink-400/10',
  ArrayAccessExpr: 'border-teal-400 bg-teal-400/10',
  VarExpr: 'border-blue-300 bg-blue-300/10',
  BuiltinExpr: 'border-yellow-500 bg-yellow-500/10',
};

function ASTNodeCard({ node, depth = 0 }: { node: ASTNode; depth?: number }) {
  const [collapsed, setCollapsed] = useState(depth > 3);
  const hasChildren = node.children.length > 0;
  const colorClass = KIND_COLORS[node.kind] || 'border-border-custom bg-surface2';

  return (
    <motion.div
      initial={{ opacity: 0, x: -10 }}
      animate={{ opacity: 1, x: 0 }}
      transition={{ delay: depth * 0.03 }}
      className="relative"
    >
      {/* Connector line */}
      {depth > 0 && (
        <div className="absolute -left-4 top-0 bottom-0 w-px bg-border-custom" />
      )}

      <div
        onClick={() => hasChildren && setCollapsed(!collapsed)}
        className={`flex items-center gap-2 px-3 py-1.5 rounded-md border text-xs
          font-mono cursor-pointer transition-all duration-150
          hover:brightness-110 ${colorClass}`}
      >
        {hasChildren && (
          <motion.span
            animate={{ rotate: collapsed ? 0 : 90 }}
            className="text-muted-foreground text-[10px]"
          >
            ▶
          </motion.span>
        )}
        <span className="font-semibold text-foreground">{node.kind}</span>
        {node.value && (
          <span className="text-nvidia">{node.value}</span>
        )}
        {node.op && (
          <span className="px-1.5 py-0.5 rounded bg-nvidia/20 text-nvidia font-bold">
            {node.op}
          </span>
        )}
        {node.type && (
          <span className="text-muted-foreground">: {node.type}</span>
        )}
        <span className="text-muted-foreground/40 ml-auto">L{node.line}</span>
      </div>

      {/* Children */}
      {hasChildren && !collapsed && (
        <div className="ml-6 mt-1 space-y-1 border-l border-border-custom/50 pl-4">
          {node.children.map((child) => (
            <ASTNodeCard key={child.id} node={child} depth={depth + 1} />
          ))}
        </div>
      )}
    </motion.div>
  );
}

export default function ASTView() {
  const result = usePipelineStore((s) => s.result);
  if (!result) return null;

  return (
    <div className="p-4 space-y-3">
      <h2 className="text-sm font-semibold text-foreground">
        Abstract Syntax Tree
        <span className="ml-2 text-muted-foreground font-normal">
          (click to expand/collapse)
        </span>
      </h2>
      <ASTNodeCard node={result.ast} />
    </div>
  );
}
