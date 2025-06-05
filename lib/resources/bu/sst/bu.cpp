#include "bu.h"

BU::BU(SST::ComponentId_t id, SST::Params &params) : DRRAResource(id, params) {}

void BU::init(unsigned int phase) {}

void BU::setup() {}

void BU::complete(unsigned int phase) {}

void BU::finish() {}

void BU::decodeInstr(uint32_t instr) {}
