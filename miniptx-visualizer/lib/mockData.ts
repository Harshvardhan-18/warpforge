import { PipelineResult } from './types';

const SOURCE = `kernel void vec_add(float* a, float* b, float* c, int n) {
    int tid = threadIdx.x + blockIdx.x * blockDim.x;
    if (tid < n) {
        c[tid] = a[tid] + b[tid];
    }
}`;

export const MOCK_RESULT: PipelineResult = {
  source: SOURCE,

  tokens: [
    { type: 'KEYWORD', value: 'kernel', line: 1, col: 1 },
    { type: 'KEYWORD', value: 'void', line: 1, col: 8 },
    { type: 'IDENT', value: 'vec_add', line: 1, col: 13 },
    { type: 'SYMBOL', value: '(', line: 1, col: 20 },
    { type: 'KEYWORD', value: 'float', line: 1, col: 21 },
    { type: 'OPERATOR', value: '*', line: 1, col: 26 },
    { type: 'IDENT', value: 'a', line: 1, col: 28 },
    { type: 'SYMBOL', value: ',', line: 1, col: 29 },
    { type: 'KEYWORD', value: 'float', line: 1, col: 31 },
    { type: 'OPERATOR', value: '*', line: 1, col: 36 },
    { type: 'IDENT', value: 'b', line: 1, col: 38 },
    { type: 'SYMBOL', value: ',', line: 1, col: 39 },
    { type: 'KEYWORD', value: 'float', line: 1, col: 41 },
    { type: 'OPERATOR', value: '*', line: 1, col: 46 },
    { type: 'IDENT', value: 'c', line: 1, col: 48 },
    { type: 'SYMBOL', value: ',', line: 1, col: 49 },
    { type: 'KEYWORD', value: 'int', line: 1, col: 51 },
    { type: 'IDENT', value: 'n', line: 1, col: 55 },
    { type: 'SYMBOL', value: ')', line: 1, col: 56 },
    { type: 'SYMBOL', value: '{', line: 1, col: 58 },
    { type: 'KEYWORD', value: 'int', line: 2, col: 5 },
    { type: 'IDENT', value: 'tid', line: 2, col: 9 },
    { type: 'OPERATOR', value: '=', line: 2, col: 13 },
    { type: 'BUILTIN', value: 'threadIdx.x', line: 2, col: 15 },
    { type: 'OPERATOR', value: '+', line: 2, col: 27 },
    { type: 'BUILTIN', value: 'blockIdx.x', line: 2, col: 29 },
    { type: 'OPERATOR', value: '*', line: 2, col: 40 },
    { type: 'BUILTIN', value: 'blockDim.x', line: 2, col: 42 },
    { type: 'SYMBOL', value: ';', line: 2, col: 52 },
    { type: 'KEYWORD', value: 'if', line: 3, col: 5 },
    { type: 'SYMBOL', value: '(', line: 3, col: 8 },
    { type: 'IDENT', value: 'tid', line: 3, col: 9 },
    { type: 'OPERATOR', value: '<', line: 3, col: 13 },
    { type: 'IDENT', value: 'n', line: 3, col: 15 },
    { type: 'SYMBOL', value: ')', line: 3, col: 16 },
    { type: 'SYMBOL', value: '{', line: 3, col: 18 },
    { type: 'IDENT', value: 'c', line: 4, col: 9 },
    { type: 'SYMBOL', value: '[', line: 4, col: 10 },
    { type: 'IDENT', value: 'tid', line: 4, col: 11 },
    { type: 'SYMBOL', value: ']', line: 4, col: 14 },
    { type: 'OPERATOR', value: '=', line: 4, col: 16 },
    { type: 'IDENT', value: 'a', line: 4, col: 18 },
    { type: 'SYMBOL', value: '[', line: 4, col: 19 },
    { type: 'IDENT', value: 'tid', line: 4, col: 20 },
    { type: 'SYMBOL', value: ']', line: 4, col: 23 },
    { type: 'OPERATOR', value: '+', line: 4, col: 25 },
    { type: 'IDENT', value: 'b', line: 4, col: 27 },
    { type: 'SYMBOL', value: '[', line: 4, col: 28 },
    { type: 'IDENT', value: 'tid', line: 4, col: 29 },
    { type: 'SYMBOL', value: ']', line: 4, col: 32 },
    { type: 'SYMBOL', value: ';', line: 4, col: 33 },
    { type: 'SYMBOL', value: '}', line: 5, col: 5 },
    { type: 'SYMBOL', value: '}', line: 6, col: 1 },
    { type: 'EOF', value: '', line: 7, col: 1 },
  ],

  ast: {
    id: 'n0', kind: 'KernelDecl', value: 'vec_add', line: 1,
    children: [
      { id: 'n1', kind: 'ParamDecl', value: 'a', type: 'float*', line: 1, children: [] },
      { id: 'n2', kind: 'ParamDecl', value: 'b', type: 'float*', line: 1, children: [] },
      { id: 'n3', kind: 'ParamDecl', value: 'c', type: 'float*', line: 1, children: [] },
      { id: 'n4', kind: 'ParamDecl', value: 'n', type: 'int', line: 1, children: [] },
      {
        id: 'n5', kind: 'BlockStmt', line: 1,
        children: [
          {
            id: 'n6', kind: 'VarDeclStmt', value: 'tid', type: 'int', line: 2,
            children: [{
              id: 'n7', kind: 'BinaryExpr', op: '+', line: 2,
              children: [
                { id: 'n8', kind: 'BuiltinExpr', value: 'threadIdx.x', line: 2, children: [] },
                {
                  id: 'n9', kind: 'BinaryExpr', op: '*', line: 2,
                  children: [
                    { id: 'n10', kind: 'BuiltinExpr', value: 'blockIdx.x', line: 2, children: [] },
                    { id: 'n11', kind: 'BuiltinExpr', value: 'blockDim.x', line: 2, children: [] },
                  ],
                },
              ],
            }],
          },
          {
            id: 'n12', kind: 'IfStmt', line: 3,
            children: [
              {
                id: 'n13', kind: 'BinaryExpr', op: '<', line: 3,
                children: [
                  { id: 'n14', kind: 'VarExpr', value: 'tid', line: 3, children: [] },
                  { id: 'n15', kind: 'VarExpr', value: 'n', line: 3, children: [] },
                ],
              },
              {
                id: 'n16', kind: 'BlockStmt', line: 3,
                children: [{
                  id: 'n17', kind: 'AssignExpr', line: 4,
                  children: [
                    {
                      id: 'n18', kind: 'ArrayAccessExpr', line: 4,
                      children: [
                        { id: 'n19', kind: 'VarExpr', value: 'c', line: 4, children: [] },
                        { id: 'n20', kind: 'VarExpr', value: 'tid', line: 4, children: [] },
                      ],
                    },
                    {
                      id: 'n21', kind: 'BinaryExpr', op: '+', line: 4,
                      children: [
                        {
                          id: 'n22', kind: 'ArrayAccessExpr', line: 4,
                          children: [
                            { id: 'n23', kind: 'VarExpr', value: 'a', line: 4, children: [] },
                            { id: 'n24', kind: 'VarExpr', value: 'tid', line: 4, children: [] },
                          ],
                        },
                        {
                          id: 'n25', kind: 'ArrayAccessExpr', line: 4,
                          children: [
                            { id: 'n26', kind: 'VarExpr', value: 'b', line: 4, children: [] },
                            { id: 'n27', kind: 'VarExpr', value: 'tid', line: 4, children: [] },
                          ],
                        },
                      ],
                    },
                  ],
                }],
              },
            ],
          },
        ],
      },
    ],
  },

  ir: {
    name: 'vec_add',
    params: [
      { name: '%a', type: 'f32*' },
      { name: '%b', type: 'f32*' },
      { name: '%c', type: 'f32*' },
      { name: '%n', type: 'i32' },
    ],
    blocks: [
      {
        label: 'entry',
        successors: ['body', 'exit'],
        instructions: [
          { id: 'i0', kind: 'MOV_BUILTIN', dst: '%t0', builtin: 'threadIdx.x', line: 2 },
          { id: 'i1', kind: 'MOV_BUILTIN', dst: '%t1', builtin: 'blockIdx.x', line: 2 },
          { id: 'i2', kind: 'MOV_BUILTIN', dst: '%t2', builtin: 'blockDim.x', line: 2 },
          { id: 'i3', kind: 'BINOP', dst: '%t3', op: 'mul', lhs: '%t1', rhs: '%t2', line: 2 },
          { id: 'i4', kind: 'BINOP', dst: '%t4', op: 'add', lhs: '%t0', rhs: '%t3', line: 2 },
          { id: 'i5', kind: 'ICMP', dst: '%t5', op: 'lt', lhs: '%t4', rhs: '%n', line: 3 },
          { id: 'i6', kind: 'BRANCH', dst: '%t5', label: 'body → exit', line: 3 },
        ],
      },
      {
        label: 'body',
        successors: ['exit'],
        instructions: [
          { id: 'i7', kind: 'LOAD', dst: '%t6', lhs: '%a', rhs: '%t4', line: 4 },
          { id: 'i8', kind: 'LOAD', dst: '%t7', lhs: '%b', rhs: '%t4', line: 4 },
          { id: 'i9', kind: 'BINOP', dst: '%t8', op: 'fadd', lhs: '%t6', rhs: '%t7', line: 4 },
          { id: 'i10', kind: 'STORE', lhs: '%c', rhs: '%t4', dst: '%t8', line: 4 },
          { id: 'i11', kind: 'JUMP', label: 'exit', line: 4 },
        ],
      },
      {
        label: 'exit',
        successors: [],
        instructions: [
          { id: 'i12', kind: 'RET', line: 6 },
        ],
      },
    ],
  },

  irOpt: {
    name: 'vec_add',
    params: [
      { name: '%a', type: 'f32*' },
      { name: '%b', type: 'f32*' },
      { name: '%c', type: 'f32*' },
      { name: '%n', type: 'i32' },
    ],
    blocks: [
      {
        label: 'entry',
        successors: ['body', 'exit'],
        instructions: [
          { id: 'i0', kind: 'MOV_BUILTIN', dst: '%t0', builtin: 'threadIdx.x', line: 2 },
          { id: 'i1', kind: 'MOV_BUILTIN', dst: '%t1', builtin: 'blockIdx.x', line: 2 },
          { id: 'i2', kind: 'MOV_BUILTIN', dst: '%t2', builtin: 'blockDim.x', line: 2 },
          { id: 'i3', kind: 'BINOP', dst: '%t3', op: 'mul', lhs: '%t1', rhs: '%t2', line: 2 },
          { id: 'i4', kind: 'BINOP', dst: '%t4', op: 'add', lhs: '%t0', rhs: '%t3', line: 2 },
          { id: 'i4b', kind: 'ASSIGN', dst: '%tid.1', lhs: '%t4', line: 2, removedByOpt: true },
          { id: 'i5', kind: 'ICMP', dst: '%t5', op: 'lt', lhs: '%t4', rhs: '%n', line: 3 },
          { id: 'i6', kind: 'BRANCH', dst: '%t5', label: 'body → exit', line: 3 },
        ],
      },
      {
        label: 'body',
        successors: ['exit'],
        instructions: [
          { id: 'i7', kind: 'LOAD', dst: '%t6', lhs: '%a', rhs: '%t4', line: 4 },
          { id: 'i8', kind: 'LOAD', dst: '%t7', lhs: '%b', rhs: '%t4', line: 4 },
          { id: 'i9', kind: 'BINOP', dst: '%t8', op: 'fadd', lhs: '%t6', rhs: '%t7', line: 4 },
          { id: 'i10', kind: 'STORE', lhs: '%c', rhs: '%t4', dst: '%t8', line: 4 },
          { id: 'i11', kind: 'JUMP', label: 'exit', line: 4 },
        ],
      },
      {
        label: 'exit',
        successors: [],
        instructions: [
          { id: 'i12', kind: 'RET', line: 6 },
        ],
      },
    ],
  },

  regalloc: {
    nodes: [
      { id: '%t0', ptxType: 's32', ptxReg: '%r0', spilled: false, useCount: 2 },
      { id: '%t1', ptxType: 's32', ptxReg: '%r1', spilled: false, useCount: 1 },
      { id: '%t2', ptxType: 's32', ptxReg: '%r2', spilled: false, useCount: 1 },
      { id: '%t3', ptxType: 's32', ptxReg: '%r1', spilled: false, useCount: 1 },
      { id: '%t4', ptxType: 's32', ptxReg: '%r0', spilled: false, useCount: 4 },
      { id: '%t5', ptxType: 'pred', ptxReg: '%p0', spilled: false, useCount: 1 },
      { id: '%t6', ptxType: 'f32', ptxReg: '%f0', spilled: false, useCount: 1 },
      { id: '%t7', ptxType: 'f32', ptxReg: '%f1', spilled: false, useCount: 1 },
      { id: '%t8', ptxType: 'f32', ptxReg: '%f0', spilled: false, useCount: 1 },
    ],
    edges: [
      { source: '%t0', target: '%t1' },
      { source: '%t0', target: '%t2' },
      { source: '%t1', target: '%t2' },
      { source: '%t3', target: '%t0' },
      { source: '%t4', target: '%t5' },
      { source: '%t6', target: '%t7' },
    ],
    registerMap: {
      '%t0': '%r0', '%t1': '%r1', '%t2': '%r2', '%t3': '%r1',
      '%t4': '%r0', '%t5': '%p0', '%t6': '%f0', '%t7': '%f1', '%t8': '%f0',
    },
    declarations: [
      '.reg .s32 %r<3>;',
      '.reg .f32 %f<2>;',
      '.reg .u64 %rd<4>;',
      '.reg .pred %p<1>;',
    ],
    spillCount: 0,
  },

  ptx: `.version 7.0
.target sm_80
.address_size 64

.file 1 "vec_add.ptxc"

.visible .entry vec_add(
    .param .u64 param_a,
    .param .u64 param_b,
    .param .u64 param_c,
    .param .s32 param_n
)
{
    .reg .s32  %r<3>;
    .reg .f32  %f<2>;
    .reg .u64  %rd<4>;
    .reg .pred %p<1>;

    ld.param.u64  %rd0, [param_a];
    ld.param.u64  %rd1, [param_b];
    ld.param.u64  %rd2, [param_c];
    ld.param.s32  %r0,  [param_n];

    .loc 1 2 0
    mov.u32       %r1, %tid.x;
    mov.u32       %r2, %ctaid.x;
    mov.u32       %r0, %ntid.x;
    mul.lo.s32    %r2, %r2, %r0;
    add.s32       %r1, %r1, %r2;

    .loc 1 3 0
    setp.lt.s32   %p0, %r1, %r0;
    @!%p0 bra     exit;

    .loc 1 4 0
    cvta.to.global.u64 %rd3, %rd0;
    mul.wide.s32  %rd3, %r1, 4;
    add.u64       %rd3, %rd0, %rd3;
    ld.global.f32 %f0, [%rd3];

    cvta.to.global.u64 %rd3, %rd1;
    mul.wide.s32  %rd3, %r1, 4;
    add.u64       %rd3, %rd1, %rd3;
    ld.global.f32 %f1, [%rd3];

    add.f32       %f0, %f0, %f1;

    cvta.to.global.u64 %rd3, %rd2;
    mul.wide.s32  %rd3, %r1, 4;
    add.u64       %rd3, %rd2, %rd3;
    st.global.f32 [%rd3], %f0;

exit:
    ret;
}`,

  stats: {
    kernelName: 'vec_add',
    instructions: 24,
    registers: { s32: 3, f32: 2, u64: 4 },
    optimisationSummary: {
      dceRemoved: 1,
      cseRemoved: 0,
      constFoldRemoved: 0,
    },
  },
};
