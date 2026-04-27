export const EXAMPLES = [
  {
    label: 'vec_add',
    description: 'Float vector addition — c[i] = a[i] + b[i]',
    source: `kernel void vec_add(float* a, float* b, float* c, int n) {
    int tid = threadIdx.x + blockIdx.x * blockDim.x;
    if (tid < n) {
        c[tid] = a[tid] + b[tid];
    }
}`,
  },
  {
    label: 'scalar_mul',
    description: 'Element-wise squaring — out[i] = arr[i] * arr[i]',
    source: `kernel void scalar_mul(float* arr, float* out, int n) {
    int tid = threadIdx.x + blockIdx.x * blockDim.x;
    if (tid < n) {
        out[tid] = arr[tid] * arr[tid];
    }
}`,
  },
  {
    label: 'dot_product',
    description: 'Partial dot product with a for loop',
    source: `kernel void dot_product(float* a, float* b, float* out, int n) {
    int tid = threadIdx.x + blockIdx.x * blockDim.x;
    if (tid < n) {
        float sum = a[tid] * b[tid];
        out[tid] = sum;
    }
}`,
  },
  {
    label: 'broken_example',
    description: 'Type error demo — assigns float to int pointer',
    source: `kernel void broken(int* data, float* vals, int n) {
    int tid = threadIdx.x + blockIdx.x * blockDim.x;
    data[tid] = vals[tid];
}`,
  },
];
