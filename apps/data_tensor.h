#pragma once

#include <array>
#include <vector>
#include <limits>


namespace dal {

    template <typename T>
    T _multipliy_elements(const T* const data, const size_t size) {
        T result = 1;

        for (size_t i = 0; i < size; ++i) {
            result *= data[i];
        }

        return result;
    }


    template <typename _DataType, uint32_t _Dimension>
    class DataTensor {

    public:
        static constexpr size_t NULL_POS = std::numeric_limits<uint32_t>::max();

    private:
        std::vector<_DataType> m_data;
        std::array<uint32_t, _Dimension> m_sizes{ 0 };

    public:
        void reset(const std::array<uint32_t, _Dimension>& sizes) {
            this->m_sizes = sizes;
            this->m_data.clear();
            this->m_data.resize( dal::_multipliy_elements<uint32_t>(sizes.data(), sizes.size()) );
        }
        void clear() {
            this->m_data.clear();
            this->m_sizes = std::array<uint32_t, _Dimension>{ 0 };
        }

        auto& at(const std::array<uint32_t, _Dimension>& indices) {
            const auto index = this->calc_stretched_index(indices, this->m_sizes);
            return this->m_data.at(index);
        }
        auto& at(const std::array<uint32_t, _Dimension>& indices) const {
            const auto index = this->calc_stretched_index(indices, this->m_sizes);
            return this->m_data.at(index);
        }

        auto& sizes() const {
            return this->m_sizes;
        }
        auto linear_size() const {
            return this->m_data.size();
        }
        auto data() {
            return this->m_data.data();
        }
        auto data() const {
            return this->m_data.data();
        }

    private:
        static uint32_t calc_stretched_index(const std::array<uint32_t, _Dimension>& indices, const std::array<uint32_t, _Dimension>& sizes) {
            uint32_t result = 0;

            for (uint32_t i = 0; i < indices.size(); ++i) {
                if (indices[i] >= sizes[i]) {
                    return NULL_POS;
                }

                const auto page_size = dal::_multipliy_elements(sizes.data(), i);
                result += indices[i] * page_size;
            }

            return result;
        }

    };

}
