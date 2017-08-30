#include "utils.h"

using namespace CoreIR;

namespace sim_core {

  bool isBitArrayOfLength(Type& t, const uint len) {
    if (t.getKind() != Type::TK_Array) {
      return false;
    }

    ArrayType& tArr = static_cast<ArrayType&>(t);

    Type::TypeKind elemKind = (tArr.getElemType())->getKind();
    if ((elemKind == Type::TK_Bit || elemKind == Type::TK_BitIn) &&
	(tArr.getLen() == len)) {
      return true;
    }

    return false;
  }

  bool isBitArrayOfLengthLEQ(Type& t, const uint len) {
    if (t.getKind() != Type::TK_Array) {
      return false;
    }

    ArrayType& tArr = static_cast<ArrayType&>(t);

    Type::TypeKind elemKind = (tArr.getElemType())->getKind();
    if ((elemKind == Type::TK_Bit || elemKind == Type::TK_BitIn) &&
	(tArr.getLen() <= len)) {
      return true;
    }

    return false;
  }
  
  bool isPrimitiveType(Type& t) {

    if (t.getKind() == Type::TK_BitIn) {
      return true;
    }
    
    if (isBitArrayOfLengthLEQ(t, 8)) {
      return true;
    }

    if (isBitArrayOfLengthLEQ(t, 16)) {
      return true;
    }
    
    if (isBitArrayOfLengthLEQ(t, 32)) {
      return true;
    }

    if (isBitArrayOfLengthLEQ(t, 64)) {
      return true;
    }

    return false;
  }

  
  std::string cTypeString(Type& t) {
    if (isBitArrayOfLengthLEQ(t, 8)) {
      return "uint8_t";
    }

    if (isBitArrayOfLengthLEQ(t, 16)) {
      return "uint16_t";
    }
    
    if (isBitArrayOfLengthLEQ(t, 32)) {
      return "uint32_t";
    }

    if (isBitArrayOfLengthLEQ(t, 64)) {
      return "uint64_t";
    }
    
    if (isArray(t)) {
      ArrayType& tArr = static_cast<ArrayType&>(t);
      Type& underlying = *(tArr.getElemType());

      return cTypeString(underlying) + "*";
    }

    if (t.getKind() == Type::TK_BitIn) {
      return "uint8_t";
    }

    if (t.getKind() == Type::TK_Bit) {
      return "uint8_t";
    }

    if (isClkIn(t) || isNamedType(t, "clk")) {
      return "uint8_t";
    }

    cout << "ERROR: Unsupported type = " << t.toString() << endl;

    assert(false);

  }

  
}
