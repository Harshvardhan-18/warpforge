export interface Token {
  type: string;
  value: string;
  line: number;
  col: number;
}

export interface ASTNode {
  id: string;
  kind: string;
  value?: string;
  type?: string;
  op?: string;
  line: number;
  children: ASTNode[];
}

export interface IRInstruction {
  id: string;
  kind: string;
  dst?: string;
  op?: string;
  lhs?: string;
  rhs?: string;
  label?: string;
  builtin?: string;
  line: number;
  removedByOpt?: boolean;
  addedBySpill?: boolean;
}

export interface BasicBlock {
  label: string;
  instructions: IRInstruction[];
  successors: string[];
}

export interface IRFunction {
  name: string;
  params: { name: string; type: string }[];
  blocks: BasicBlock[];
}

export interface InterferenceNode {
  id: string;
  ptxType: string;
  ptxReg: string;
  spilled: boolean;
  useCount: number;
  x?: number;
  y?: number;
}

export interface InterferenceEdge {
  source: string;
  target: string;
}

export interface RegAllocResult {
  nodes: InterferenceNode[];
  edges: InterferenceEdge[];
  registerMap: Record<string, string>;
  declarations: string[];
  spillCount: number;
}

export interface CompileStats {
  kernelName: string;
  instructions: number;
  registers: { s32: number; f32: number; u64: number };
  optimisationSummary: {
    dceRemoved: number;
    cseRemoved: number;
    constFoldRemoved: number;
  };
}

export interface PipelineResult {
  source: string;
  tokens: Token[];
  ast: ASTNode;
  ir: IRFunction;
  irOpt: IRFunction;
  regalloc: RegAllocResult;
  ptx: string;
  stats: CompileStats;
}

export type StageKey = 'tokens' | 'ast' | 'ir' | 'irOpt' | 'regalloc' | 'ptx';
export type CompileStatus = 'idle' | 'compiling' | 'done' | 'error';
