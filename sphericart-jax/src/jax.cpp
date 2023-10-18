// This file defines the Python interface to the XLA custom calls implemented on
// the CPU. It is exposed as a standard pybind11 module defining "capsule"
// objects containing our methods. For simplicity, we export a separate capsule
// for each supported dtype.

#include <cstdlib>

#include "sphericart.hpp"
#include "sphericart/pybind11_kernel_helpers.h"

using namespace sphericart_jax;

namespace {

template <typename T> void cpu_sph(void *out, const void **in) {
    // Parse the inputs
    const T *xyz = reinterpret_cast<const T *>(in[0]);
    const size_t l_max = *reinterpret_cast<const int *>(in[1]);
    const bool normalized = *reinterpret_cast<const bool *>(in[2]);
    const size_t n_samples = *reinterpret_cast<const int *>(in[3]);
    size_t xyz_length{n_samples * 3};
    size_t sph_len{(l_max + 1) * (l_max + 1) * n_samples};
    // The output is stored as a single pointer since there is only one output
    T *sph = reinterpret_cast<T *>(out);

    auto calculator = sphericart::SphericalHarmonics<T>(l_max, normalized);
    calculator.compute_array(xyz, xyz_length, sph, sph_len);
}

template <typename T>
void cpu_sph_with_gradients(void *out_tuple, const void **in) {
    // Parse the inputs
    const T *xyz = reinterpret_cast<const T *>(in[0]);
    const size_t l_max = *reinterpret_cast<const int *>(in[1]);
    const bool normalized = *reinterpret_cast<const bool *>(in[2]);
    const size_t n_samples = *reinterpret_cast<const int *>(in[3]);
    size_t xyz_length{n_samples * 3};
    size_t sph_len{(l_max + 1) * (l_max + 1) * n_samples};
    size_t dsph_len{sph_len * 3};
    // The output is stored as a list of pointers since we have multiple outputs
    void **out = reinterpret_cast<void **>(out_tuple);
    T *sph = reinterpret_cast<T *>(out[0]);
    T *dsph = reinterpret_cast<T *>(out[1]);

    auto calculator = sphericart::SphericalHarmonics<T>(l_max, normalized);
    calculator.compute_array_with_gradients(xyz, xyz_length, sph, sph_len, dsph,
                                            dsph_len);
}

template <typename T>
void cpu_sph_with_hessians(void *out_tuple, const void **in) {
    // Parse the inputs
    const T *xyz = reinterpret_cast<const T *>(in[0]);
    const size_t l_max = *reinterpret_cast<const int *>(in[1]);
    const bool normalized = *reinterpret_cast<const bool *>(in[2]);
    const size_t n_samples = *reinterpret_cast<const int *>(in[3]);
    size_t xyz_length{n_samples * 3};
    size_t sph_len{(l_max + 1) * (l_max + 1) * n_samples};
    size_t dsph_len{sph_len * 3};
    size_t ddsph_len{sph_len * 3 * 3};
    // The output is stored as a list of pointers since we have multiple outputs
    void **out = reinterpret_cast<void **>(out_tuple);
    T *sph = reinterpret_cast<T *>(out[0]);
    T *dsph = reinterpret_cast<T *>(out[1]);
    T *ddsph = reinterpret_cast<T *>(out[2]);

    auto calculator = sphericart::SphericalHarmonics<T>(l_max, normalized);
    calculator.compute_array_with_hessians(xyz, xyz_length, sph, sph_len, dsph,
                                           dsph_len, ddsph, ddsph_len);
}

pybind11::dict Registrations() {
    pybind11::dict dict;
    dict["cpu_sph_f32"] = EncapsulateFunction(cpu_sph<float>);
    dict["cpu_sph_f64"] = EncapsulateFunction(cpu_sph<double>);
    dict["cpu_dsph_f32"] = EncapsulateFunction(cpu_sph_with_gradients<float>);
    dict["cpu_dsph_f64"] = EncapsulateFunction(cpu_sph_with_gradients<double>);
    dict["cpu_ddsph_f32"] = EncapsulateFunction(cpu_sph_with_hessians<float>);
    dict["cpu_ddsph_f64"] = EncapsulateFunction(cpu_sph_with_hessians<double>);
    return dict;
}

PYBIND11_MODULE(sphericart_jax_cpu, m) {
    m.def("registrations", &Registrations);
}

} // namespace