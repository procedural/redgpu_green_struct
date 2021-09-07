/*
LD_LIBRARY_PATH=/opt/RedGpuSDK/clspv /opt/RedGpuSDK/clspv/clspv add.cl -o add_not_stripped.cl.spv
LD_LIBRARY_PATH=/opt/RedGpuSDK/clspv /opt/RedGpuSDK/clspv/clspv-reflection add_not_stripped.cl.spv -o add.cl.map
/opt/RedGpuSDK/sdk/1.2.135.0/x86_64/bin/spirv-opt --strip-reflect add_not_stripped.cl.spv -o add.cl.spv
rm add_not_stripped.cl.spv
*/

kernel void mainAdd(global float4 * a, global float4 * b, global float4 * c) {
  c[0] = a[0] + b[0];
}
