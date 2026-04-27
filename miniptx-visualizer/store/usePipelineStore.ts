import { create } from 'zustand';
import { compilePTXC } from '@/lib/api';
import type { StageKey, CompileStatus, PipelineResult } from '@/lib/types';

interface PipelineState {
  source: string;
  status: CompileStatus;
  activeStage: StageKey;
  result: PipelineResult | null;
  error: string | null;
  compareMode: boolean;
  sourceB: string;
  resultB: PipelineResult | null;
  statusB: CompileStatus;

  setSource: (s: string) => void;
  setSourceB: (s: string) => void;
  setActiveStage: (s: StageKey) => void;
  setCompareMode: (v: boolean) => void;
  reset: () => void;
  compile: () => Promise<void>;
  compileB: () => Promise<void>;
}

const DEFAULT_SOURCE = `kernel void vec_add(float* a, float* b, float* c, int n) {
    int tid = threadIdx.x + blockIdx.x * blockDim.x;
    if (tid < n) {
        c[tid] = a[tid] + b[tid];
    }
}`;

export const usePipelineStore = create<PipelineState>((set, get) => ({
  source: DEFAULT_SOURCE,
  status: 'idle',
  activeStage: 'tokens',
  result: null,
  error: null,
  compareMode: false,
  sourceB: '',
  resultB: null,
  statusB: 'idle',

  setSource: (s) => set({ source: s }),
  setSourceB: (s) => set({ sourceB: s }),
  setActiveStage: (s) => set({ activeStage: s }),
  setCompareMode: (v) => set({ compareMode: v }),

  reset: () =>
    set({ result: null, error: null, status: 'idle', resultB: null, statusB: 'idle' }),

  compile: async () => {
    set({ status: 'compiling', error: null });
    try {
      const result = await compilePTXC(get().source);
      set({ result, status: 'done' });
    } catch (e: unknown) {
      const msg = e instanceof Error ? e.message : 'Unknown error';
      set({ error: msg, status: 'error' });
    }
  },

  compileB: async () => {
    set({ statusB: 'compiling' });
    try {
      const resultB = await compilePTXC(get().sourceB);
      set({ resultB, statusB: 'done' });
    } catch (e: unknown) {
      const msg = e instanceof Error ? e.message : 'Unknown error';
      set({ error: msg, statusB: 'error' });
    }
  },
}));
