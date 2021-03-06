#ifndef DYNET_CUDA_MATRIX_MULTIPLY_H__
#define DYNET_CUDA_MATRIX_MULTIPLY_H__

#include "dynet/tensor.h"
#include "dynet/devices.h"
#include "dynet/dynet.h"
#include "dynet/nodes-macros.h"

#ifdef __CUDACC__

#include "dynet/cuda.h"

namespace dynet {

inline void MatrixMultiply(const Device_GPU & dev, const Tensor& l, const Tensor& r, Tensor& y, const float* acc_scalar) {
  CUDA_CHECK(cudaSetDevice(dev.cuda_device_id));
  if(l.d.bd == 1 && r.d.bd == y.d.bd) {
    // If the left side has one batch, multiply by columns
    // [x, z, b] = [x, y] * [y, z, b]
    // -> [x, z*b] = [x, y], [y, z*b]
    CUBLAS_CHECK(cublasSgemm(dev.cublas_handle, CUBLAS_OP_N, CUBLAS_OP_N,
          y.d.rows(), y.d.cols() * y.d.batch_elems(), l.d.cols(),
          dev.kSCALAR_ONE,
          l.v, l.d.rows(),
          r.v, r.d.rows(),
          acc_scalar, y.v, y.d.rows()));
  } else {
    // Otherwise, loop over the batches
    for(unsigned b = 0; b < y.d.bd; ++b) {
      CUBLAS_CHECK(cublasSgemm(dev.cublas_handle, CUBLAS_OP_N, CUBLAS_OP_N,
            y.d.rows(), y.d.cols(), l.d.cols(),
            dev.kSCALAR_ONE,
            l.batch_ptr(b), l.d.rows(),
            r.batch_ptr(b), r.d.rows(),
            acc_scalar, y.batch_ptr(b), y.d.rows()));
    }
  }
}

}

#else

namespace dynet {

inline void MatrixMultiply(const Device_CPU & dev, const Tensor& l, const Tensor& r, Tensor& y, const float* acc_scalar) {

  y.tbvec().device(*dev.edevice) = *acc_scalar * y.tbvec();

  if(l.d.bd == 1 && r.d.bd == y.d.bd) {

      // If the left side has one batch, multiply by columns
      // [x, z, b] = [x, y] * [y, z, b]
      // -> [x, z*b] = [x, y], [y, z*b]
      y.colbatch_matrix().noalias() += *l * r.colbatch_matrix();

  } else {
    // Otherwise, loop over the batches
    for(unsigned b = 0; b < y.d.bd; ++b)
      y.batch_matrix(b).noalias() += l.batch_matrix(b) * r.batch_matrix(b);

  }
}

}

#endif

#ifdef __CUDACC__
inline void MatrixTranspMultiplyAcc(const dynet::Device_GPU & dev, const dynet::Tensor& l, const dynet::Tensor& r, dynet::Tensor& y) {
  // computes l^T * r
  int max_b = std::max(l.d.bd, r.d.bd);
  // Do a single multiply if l has one batch
  if(l.d.bd == 1 && y.d.bd == r.d.bd) {
    CUBLAS_CHECK(cublasSgemm(dev.cublas_handle, CUBLAS_OP_T, CUBLAS_OP_N,
          y.d.rows(), y.d.cols()*y.d.batch_elems(), l.d.rows(),
          dev.kSCALAR_ONE,
          l.v, l.d.rows(),
          r.v, r.d.rows(),
          dev.kSCALAR_ONE, y.v, y.d.rows()));
  } else {
    for(int b = 0; b < max_b; ++b)
      CUBLAS_CHECK(cublasSgemm(dev.cublas_handle, CUBLAS_OP_T, CUBLAS_OP_N,
            y.d.rows(), y.d.cols(), l.d.rows(),
            dev.kSCALAR_ONE,
            l.batch_ptr(b), l.d.rows(),
            r.batch_ptr(b), r.d.rows(),
            dev.kSCALAR_ONE, y.batch_ptr(b), y.d.rows()));
  }
}

# else
inline void MatrixTranspMultiplyAcc(const dynet::Device_CPU & dev, const dynet::Tensor& l, const dynet::Tensor& r, dynet::Tensor& y) {
  // computes l^T * r
  int max_b = std::max(l.d.bd, r.d.bd);
  if(l.d.bd == 1 && y.d.bd == r.d.bd) {
    y.colbatch_matrix().noalias() += (*l).transpose() * r.colbatch_matrix();
  } else {
    for(int b = 0; b < max_b; ++b)
      y.batch_matrix(b).noalias() += l.batch_matrix(b).transpose() * r.batch_matrix(b);
  }
}
#endif

#ifdef __CUDACC__
inline void MatrixMultiplyTranspAcc(const dynet::Device_GPU & dev, const dynet::Tensor& l, const dynet::Tensor& r, dynet::Tensor& y) {
  int max_b = std::max(l.d.bd, r.d.bd);
  if(y.d.bd == 1 && (l.d.bd == r.d.bd)) {
    CUBLAS_CHECK(cublasSgemm(dev.cublas_handle, CUBLAS_OP_N, CUBLAS_OP_T,
          y.d.rows(), y.d.cols(), l.d.cols() * l.d.batch_elems(),
          dev.kSCALAR_ONE,
          l.v, l.d.rows(),
          r.v, r.d.rows(),
          dev.kSCALAR_ONE, y.v, y.d.rows()));
  } else {
    for(int b = 0; b < max_b; ++b)
      CUBLAS_CHECK(cublasSgemm(dev.cublas_handle, CUBLAS_OP_N, CUBLAS_OP_T,
            y.d.rows(), y.d.cols(), l.d.cols(),
            dev.kSCALAR_ONE,
            l.batch_ptr(b), l.d.rows(),
            r.batch_ptr(b), r.d.rows(),
            dev.kSCALAR_ONE, y.batch_ptr(b), y.d.rows()));
  }
}
# else
inline void MatrixMultiplyTranspAcc(const dynet::Device_CPU & dev, const dynet::Tensor& l, const dynet::Tensor& r, dynet::Tensor& y) {
  int max_b = std::max(l.d.bd, r.d.bd);
  if(y.d.bd == 1 && (l.d.bd == r.d.bd)) {
    (*y).noalias() += l.colbatch_matrix() * r.colbatch_matrix().transpose();
  } else {
    for(int b = 0; b < max_b; ++b)
      y.batch_matrix(b).noalias() += l.batch_matrix(b) * r.batch_matrix(b).transpose();
  }
}
#endif


#endif
