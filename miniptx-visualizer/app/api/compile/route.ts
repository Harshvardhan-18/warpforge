import { NextResponse } from 'next/server';
import { MOCK_RESULT } from '@/lib/mockData';

export async function POST(request: Request) {
  try {
    const body = await request.json();
    const { source } = body;

    if (!source || typeof source !== 'string' || source.trim().length === 0) {
      return NextResponse.json(
        { error: 'source must be a non-empty string' },
        { status: 400 }
      );
    }

    // Mock mode: return mock data with simulated delay
    const isMock =
      process.env.MOCK_MODE === 'true' || !process.env.MINIPTX_BIN;

    if (isMock) {
      await new Promise((resolve) => setTimeout(resolve, 1400));
      return NextResponse.json({ ...MOCK_RESULT, source });
    }

    // Real mode: spawn the MiniPTX compiler
    const crypto = await import('crypto');
    const fs = await import('fs/promises');
    const { spawn } = await import('child_process');
    const path = await import('path');
    const os = await import('os');

    const tmpDir = os.tmpdir();
    const tmpFile = path.join(
      tmpDir,
      `miniptx_${crypto.randomUUID()}.ptxc`
    );

    await fs.writeFile(tmpFile, source, 'utf-8');

    const result = await new Promise<string>((resolve, reject) => {
      const args = [
        '--emit-tokens',
        '--emit-ast',
        '--emit-ir',
        '--emit-ir-opt',
        '--emit-regalloc',
        '--emit-ptx',
        '--json',
        tmpFile,
      ];

      const proc = spawn(process.env.MINIPTX_BIN!, args);

      let stdout = '';
      let stderr = '';

      proc.stdout.on('data', (data: Buffer) => {
        stdout += data.toString();
      });
      proc.stderr.on('data', (data: Buffer) => {
        stderr += data.toString();
      });

      proc.on('close', (code: number | null) => {
        if (code !== 0) {
          reject(new Error(stderr || `miniptx exited with code ${code}`));
        } else {
          resolve(stdout);
        }
      });

      proc.on('error', (err: Error) => {
        reject(new Error(`Failed to spawn miniptx: ${err.message}`));
      });
    });

    // Cleanup tmp file
    await fs.unlink(tmpFile).catch(() => {});

    const parsed = JSON.parse(result);
    return NextResponse.json(parsed);
  } catch (error: unknown) {
    const message =
      error instanceof Error ? error.message : 'Internal server error';
    console.error('[/api/compile]', message);
    return NextResponse.json({ error: message }, { status: 500 });
  }
}
