import { PipelineResult } from './types';

export async function compilePTXC(source: string): Promise<PipelineResult> {
  const res = await fetch('/api/compile', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ source }),
  });

  const data = await res.json();

  if (!res.ok) {
    throw new Error(data.error || `Compilation failed (${res.status})`);
  }

  return data as PipelineResult;
}
