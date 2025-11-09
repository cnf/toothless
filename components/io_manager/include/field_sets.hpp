#pragma once
// TODO: figure field sets out later
#include <bitset>
// #include <cstddef>
#include <cstdint>

struct FieldPosition {
  size_t position;
  size_t width;
};
template <size_t N> class FieldSet : public std::bitset<N> {
public:
  class reference {
    FieldSet &parent;
    FieldPosition pos;

  public:
    reference(FieldSet<N> &p, FieldPosition fp) : parent(p), pos(fp) {}
    reference &operator=(uint8_t value) {
      for (size_t i = 0; i < pos.width; ++i)
        parent.std::bitset<N>::operator[](pos.position + i) = (value >> i) & 1;
      return *this;
    }
    operator uint8_t() const {
      uint8_t r = 0;
      for (size_t i = 0; i < pos.width; ++i)
        r |= (parent.std::bitset<N>::operator[](pos.position + i) << i);
      return r;
    }
    operator uint16_t() const {
      uint16_t r = 0;
      for (size_t i = 0; i < pos.width; ++i)
        r |= (parent.std::bitset<N>::operator[](pos.position + i) << i);
      return r;
    }
  };

  reference operator[](const FieldPosition &pos) { return reference(*this, pos); }
  uint8_t get_u8(const FieldPosition &pos) const {
    uint8_t r = this->std::bitset<N>::operator[](pos).uint8_t();
    // for (size_t i = 0; i < pos.width; ++i)
    //   r |= (this->std::bitset<N>::operator[](pos.position + i) << i);
    return r;
  }
};

/*
int main() {
    fieldset<16> myset{0b111110110110100};
    // FieldPosition trypos = {12,4};
    FieldPosition trypos = {9,3};
    uint8_t setres = myset[trypos];
    printf("0x%X <> 0x%X\n", setres, myset[trypos]);
    myset[trypos] = 0b110;
    setres = myset[trypos];
    printf("0x%X <> 0x%X\n", setres, myset[trypos]);


    return 0;
}
*/