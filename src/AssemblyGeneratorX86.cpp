/*
Copyright (c) 2018 tevador

This file is part of RandomX.

RandomX is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RandomX is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RandomX.  If not, see<http://www.gnu.org/licenses/>.
*/
//#define TRACE
#include "AssemblyGeneratorX86.hpp"
#include "Pcg32.hpp"
#include "common.hpp"
#include "instructions.hpp"

namespace RandomX {

	static const char* regR[8] = { "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };
	static const char* regR32[8] = { "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };
	static const char* regF[8] = { "xmm8", "xmm9", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7" };

	static const char* regMx = "rbp";
	static const char* regIc = "ebx";
	static const char* regIc8 = "bl";
	static const char* regStackBeginAddr = "rdi";
	static const char* regScratchpadAddr = "rsi";

	void AssemblyGeneratorX86::generateProgram(const void* seed) {
		asmCode.str(std::string()); //clear
		Pcg32 gen(seed);
		for (unsigned i = 0; i < sizeof(RegisterFile) / sizeof(Pcg32::result_type); ++i) {
			gen();
		}
		Instruction instr;
		for (unsigned i = 0; i < ProgramLength; ++i) {
			for (unsigned j = 0; j < sizeof(instr) / sizeof(Pcg32::result_type); ++j) {
				*(((uint32_t*)&instr) + j) = gen();
			}
			generateCode(instr, i);
			asmCode << std::endl;
		}
		if(ProgramLength > 0)
			asmCode << "\tjmp rx_i_0" << std::endl;
	}

	void AssemblyGeneratorX86::generateCode(Instruction& instr, int i) {
		asmCode << "rx_i_" << i << ": ;" << instr.getName() << std::endl;
		asmCode << "\tdec " << regIc << std::endl;
		asmCode << "\tjz rx_finish" << std::endl;
		auto generator = engine[instr.opcode];
		(this->*generator)(instr, i);
	}

	void AssemblyGeneratorX86::genar(Instruction& instr, int i) {
		asmCode << "\txor " << regR[instr.rega % RegistersCount] << ", 0" << std::hex << instr.addra << "h" << std::dec << std::endl;
		asmCode << "\tmov ecx, " << regR32[instr.rega % RegistersCount] << std::endl;
		asmCode << "\ttest " << regIc8 << ", 63" << std::endl;
		asmCode << "\tjnz short rx_body_" << i << std::endl;
		switch (instr.loca & 3)
		{
			case 0:
			case 1:
			case 2:
				asmCode << "\tcall rx_read_l1" << std::endl;
				asmCode << "rx_body_" << i << ":" << std::endl;
				if ((instr.loca & 192) == 0)
					asmCode << "\txor " << regMx << ", rcx" << std::endl;
				asmCode << "\tand ecx, " << (ScratchpadL1 - 1) << std::endl;
				break;
			default: //3
				asmCode << "\tcall rx_read_l2" << std::endl;
				asmCode << "rx_body_" << i << ":" << std::endl;
				if ((instr.loca & 192) == 0)
					asmCode << "\txor " << regMx << ", rcx" << std::endl;
				asmCode << "\tand ecx, " << (ScratchpadL2 - 1) << std::endl;
				break;
		}
		asmCode << "\tmov rax, qword ptr [" << regScratchpadAddr << "+rcx*8]" << std::endl;
	}


	void AssemblyGeneratorX86::genaf(Instruction& instr, int i) {
		asmCode << "\txor " << regR[instr.rega % RegistersCount] << ", 0" << std::hex << instr.addra << "h" << std::dec << std::endl;
		asmCode << "\tmov ecx, " << regR32[instr.rega % RegistersCount] << std::endl;
		asmCode << "\ttest " << regIc8 << ", 63" << std::endl;
		asmCode << "\tjnz short rx_body_" << i << std::endl;
		switch (instr.loca & 3)
		{
			case 0:
			case 1:
			case 2:
				asmCode << "\tcall rx_read_l1" << std::endl;
				asmCode << "rx_body_" << i << ":" << std::endl;
				if((instr.loca & 192) == 0)
					asmCode << "\txor " << regMx << ", rcx" << std::endl;
				asmCode << "\tand ecx, " << (ScratchpadL1 - 1) << std::endl;
				break;
			default: //3
				asmCode << "\tcall rx_read_l2" << std::endl;
				asmCode << "rx_body_" << i << ":" << std::endl;
				if ((instr.loca & 192) == 0)
					asmCode << "\txor " << regMx << ", rcx" << std::endl;
				asmCode << "\tand ecx, " << (ScratchpadL2 - 1) << std::endl;
				break;
		}
		asmCode << "\tcvtdq2pd xmm0, qword ptr [" << regScratchpadAddr << "+rcx*8]" << std::endl;
	}

	void AssemblyGeneratorX86::genbr0(Instruction& instr, const char* instrx86) {
		switch (instr.locb & 7)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			asmCode << "\tmov rcx, " << regR[instr.regb % RegistersCount] << std::endl;
			asmCode << "\t" << instrx86 << " rax, cl" << std::endl;
			return;
		default:
			asmCode << "\t" << instrx86 << " rax, " << (instr.imm8 & 63) << std::endl;;
			return;
		}
	}

	void AssemblyGeneratorX86::genbr1(Instruction& instr) {
		switch (instr.locb & 7)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			asmCode << regR[instr.regb % RegistersCount] << std::endl;
			return;
		default:
			asmCode  << instr.imm32 << std::endl;;
			return;
		}
	}

	void AssemblyGeneratorX86::genbr132(Instruction& instr) {
		switch (instr.locb & 7)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			asmCode << regR32[instr.regb % RegistersCount] << std::endl;
			return;
		default:
			asmCode << instr.imm32 << std::endl;;
			return;
		}
	}

	void AssemblyGeneratorX86::genbf(Instruction& instr, const char* instrx86) {
		asmCode << "\t" << instrx86 << " xmm0, " << regF[instr.regb % RegistersCount] << std::endl;
	}

	void AssemblyGeneratorX86::gencr(Instruction& instr, bool rax = true) {
		switch (instr.locc & 7)
		{
		case 0:
			if(rax)
				asmCode << "\tmov rcx, rax" << std::endl;
			asmCode << "\tmov eax, " << regR32[instr.regc % RegistersCount] << std::endl;
			asmCode << "\txor eax, 0" << std::hex << instr.addrc << "h" << std::dec << std::endl;
			asmCode << "\tand eax, " << (ScratchpadL2 - 1) << std::endl;
			asmCode << "\tmov qword ptr [" << regScratchpadAddr << " + rax * 8], rcx" << std::endl;
			if (trace) {
				asmCode << "\tmov qword ptr [" << regScratchpadAddr << " + " << regIc << " * 8 + 262136], rcx" << std::endl;
			}
			return;

		case 1:
		case 2:
		case 3:
			if (rax)
				asmCode << "\tmov rcx, rax" << std::endl;
			asmCode << "\tmov eax, " << regR32[instr.regc % RegistersCount] << std::endl;
			asmCode << "\txor eax, 0" << std::hex << instr.addrc << "h" << std::dec << std::endl;
			asmCode << "\tand eax, " << (ScratchpadL1 - 1) << std::endl;
			asmCode << "\tmov qword ptr [" << regScratchpadAddr << " + rax * 8], rcx" << std::endl;
			if (trace) {
				asmCode << "\tmov qword ptr [" << regScratchpadAddr << " + " << regIc << " * 8 + 262136], rcx" << std::endl;
			}
			return;

		default:
			asmCode << "\tmov " << regR[instr.regc % RegistersCount] << ", " << (rax ? "rax" : "rcx") << std::endl;
			if (trace) {
				asmCode << "\tmov qword ptr [" << regScratchpadAddr << " + " << regIc << " * 8 + 262136], " << (rax ? "rax" : "rcx") << std::endl;
			}
			return;
		}
	}

	void AssemblyGeneratorX86::gencf(Instruction& instr, bool move = true) {
		if(move)
			asmCode << "\tmovaps " << regF[instr.regc % RegistersCount] << ", xmm0" << std::endl;
		const char* store = (instr.locc & 128) ? "movhpd" : "movlpd";
		switch (instr.locc & 7)
		{
			case 4:
				asmCode << "\tmov eax, " << regR32[instr.regc % RegistersCount] << std::endl;
				asmCode << "\txor eax, 0" << std::hex << instr.addrc << "h" << std::dec << std::endl;
				asmCode << "\tand eax, " << (ScratchpadL2 - 1) << std::endl;
				asmCode << "\t" << store << " qword ptr [" << regScratchpadAddr << " + rax * 8], " << regF[instr.regc % RegistersCount] << std::endl;
				break;

			case 5:
			case 6:
			case 7:
				asmCode << "\tmov eax, " << regR32[instr.regc % RegistersCount] << std::endl;
				asmCode << "\txor eax, 0" << std::hex << instr.addrc << "h" << std::dec << std::endl;
				asmCode << "\tand eax, " << (ScratchpadL1 - 1) << std::endl;
				asmCode << "\t" << store << " qword ptr [" << regScratchpadAddr << " + rax * 8], " << regF[instr.regc % RegistersCount] << std::endl;
				break;
		}
		if (trace) {
			asmCode << "\t" << store << " qword ptr [" << regScratchpadAddr << " + " << regIc << " * 8 + 262136], " << regF[instr.regc % RegistersCount] << std::endl;
		}
	}

	void AssemblyGeneratorX86::h_ADD_64(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tadd rax, ";
		genbr1(instr);
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_ADD_32(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tadd eax, ";
		genbr132(instr);
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_SUB_64(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tsub rax, ";
		genbr1(instr);
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_SUB_32(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tsub eax, ";
		genbr132(instr);
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_MUL_64(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\timul rax, ";
		if ((instr.locb & 7) >= 6) {
			asmCode << "rax, ";
		}
		genbr1(instr);
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_MULH_64(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tmov rcx, ";
		genbr1(instr);
		asmCode << "\tmul rcx" << std::endl;
		asmCode << "\tmov rax, rdx" << std::endl;
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_MUL_32(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tmov ecx, eax" << std::endl;
		asmCode << "\tmov eax, ";
		genbr132(instr);
		asmCode << "\timul rax, rcx" << std::endl;
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_IMUL_32(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tmovsxd rcx, eax" << std::endl;
		if ((instr.locb & 7) >= 6) {
			asmCode << "\tmov rax, " << instr.imm32 << std::endl;
		}
		else {
			asmCode << "\tmovsxd rax, " << regR32[instr.regb % RegistersCount] << std::endl;
		}
		asmCode << "\timul rax, rcx" << std::endl;
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_IMULH_64(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tmov rcx, ";
		genbr1(instr);
		asmCode << "\timul rcx" << std::endl;
		asmCode << "\tmov rax, rdx" << std::endl;
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_DIV_64(Instruction& instr, int i) {
		genar(instr, i);
		if ((instr.locb & 7) >= 6) {
			if (instr.imm32 == 0) {
				asmCode << "\tmov ecx, 1" << std::endl;
			}
			else {
				asmCode << "\tmov ecx, " << instr.imm32 << std::endl;
			}
		}
		else {
			asmCode << "\tmov ecx, 1" << std::endl;
			asmCode << "\tmov edx, " << regR32[instr.regb % RegistersCount] << std::endl;
			asmCode << "\ttest edx, edx" << std::endl;
			asmCode << "\tcmovne ecx, edx" << std::endl;
		}
		asmCode << "\txor edx, edx" << std::endl;
		asmCode << "\tdiv rcx" << std::endl;
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_IDIV_64(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tmov edx, ";
		genbr132(instr);
		asmCode << "\tcmp edx, -1" << std::endl;
		asmCode << "\tjne short safe_idiv_" << i << std::endl;
		asmCode << "\tmov rcx, rax" << std::endl;
		asmCode << "\trol rcx, 1" << std::endl;
		asmCode << "\tdec rcx" << std::endl;
		asmCode << "\tjz short result_idiv_" << i << std::endl;
		asmCode << "safe_idiv_" << i << ":" << std::endl;
		asmCode << "\tmov ecx, 1" << std::endl;
		asmCode << "\ttest edx, edx" << std::endl;
		asmCode << "\tcmovne ecx, edx" << std::endl;
		asmCode << "\tmovsxd rcx, ecx" << std::endl;
		asmCode << "\tcqo" << std::endl;
		asmCode << "\tidiv rcx" << std::endl;
		asmCode << "result_idiv_" << i << ":" << std::endl;
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_AND_64(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tand rax, ";
		genbr1(instr);
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_AND_32(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tand eax, ";
		genbr132(instr);
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_OR_64(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tor rax, ";
		genbr1(instr);
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_OR_32(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tor eax, ";
		genbr132(instr);
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_XOR_64(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\txor rax, ";
		genbr1(instr);
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_XOR_32(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\txor eax, ";
		genbr132(instr);
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_SHL_64(Instruction& instr, int i) {
		genar(instr, i);
		genbr0(instr, "shl");
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_SHR_64(Instruction& instr, int i) {
		genar(instr, i);
		genbr0(instr, "shr");
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_SAR_64(Instruction& instr, int i) {
		genar(instr, i);
		genbr0(instr, "sar");
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_ROL_64(Instruction& instr, int i) {
		genar(instr, i);
		genbr0(instr, "rol");
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_ROR_64(Instruction& instr, int i) {
		genar(instr, i);
		genbr0(instr, "ror");
		gencr(instr);
	}

	void AssemblyGeneratorX86::h_FPADD(Instruction& instr, int i) {
		genaf(instr, i);
		genbf(instr, "addpd");
		gencf(instr);
	}

	void AssemblyGeneratorX86::h_FPSUB(Instruction& instr, int i) {
		genaf(instr, i);
		genbf(instr, "subpd");
		gencf(instr);
	}

	void AssemblyGeneratorX86::h_FPMUL(Instruction& instr, int i) {
		genaf(instr, i);
		genbf(instr, "mulpd");
		asmCode << "\tmovaps xmm1, xmm0" << std::endl;
		asmCode << "\tcmpeqpd xmm1, xmm1" << std::endl;
		asmCode << "\tandps xmm0, xmm1" << std::endl;
		gencf(instr);
	}

	void AssemblyGeneratorX86::h_FPDIV(Instruction& instr, int i) {
		genaf(instr, i);
		genbf(instr, "divpd");
		asmCode << "\tmovaps xmm1, xmm0" << std::endl;
		asmCode << "\tcmpeqpd xmm1, xmm1" << std::endl;
		asmCode << "\tandps xmm0, xmm1" << std::endl;
		gencf(instr);
	}

	void AssemblyGeneratorX86::h_FPSQRT(Instruction& instr, int i) {
		genaf(instr, i);
		asmCode << "\tandps xmm0, xmm10" << std::endl;
		asmCode << "\tsqrtpd " << regF[instr.regc % RegistersCount] << ", xmm0" << std::endl;
		gencf(instr, false);
	}

	void AssemblyGeneratorX86::h_FPROUND(Instruction& instr, int i) {
		genar(instr, i);
		asmCode << "\tmov rcx, rax" << std::endl;
		int rotate = (13 - (instr.imm8 & 63)) & 63;
		if (rotate != 0)
			asmCode << "\trol rax, " << rotate << std::endl;
		asmCode << "\tand eax, 24576" << std::endl;
		asmCode << "\tor eax, 40896" << std::endl;
		asmCode << "\tmov dword ptr [rsp - 8], eax" << std::endl;
		asmCode << "\tldmxcsr dword ptr [rsp - 8]" << std::endl;
		gencr(instr, false);
	}

	static inline const char* jumpCondition(Instruction& instr, bool invert = false) {
		switch ((instr.locb & 7) ^ invert)
		{
			case 0:
				return "jbe";
			case 1:
				return "ja";
			case 2:
				return "js";
			case 3:
				return "jns";
			case 4:
				return "jo";
			case 5:
				return "jno";
			case 6:
				return "jl";
			case 7:
				return "jge";
		}
	}

	void AssemblyGeneratorX86::h_JUMP(Instruction& instr, int i) {
		genar(instr, i);
		gencr(instr);
		asmCode << "\tcmp " << regR32[instr.regb % RegistersCount] << ", " << instr.imm32 << std::endl;
		asmCode << "\t" << jumpCondition(instr);
		asmCode << " rx_i_" << wrapInstr(i + (instr.imm8 & 127) + 2) << std::endl;
	}

	void AssemblyGeneratorX86::h_CALL(Instruction& instr, int i) {
		genar(instr, i);
		gencr(instr);
		asmCode << "\tcmp " << regR32[instr.regb % RegistersCount] << ", " << instr.imm32 << std::endl;
		asmCode << "\t" << jumpCondition(instr, true);
		asmCode << " short rx_i_" << wrapInstr(i + 1) << std::endl;
		asmCode << "\tcall rx_i_" << wrapInstr(i + (instr.imm8 & 127) + 2) << std::endl;
	}

	void AssemblyGeneratorX86::h_RET(Instruction& instr, int i) {
		genar(instr, i);
		gencr(instr);
		asmCode << "\tcmp rsp, " << regStackBeginAddr << std::endl;
		asmCode << "\tje short rx_i_" << wrapInstr(i + 1) << std::endl;
		asmCode << "\tret" << std::endl;
	}

#include "instructionWeights.hpp"
#define INST_HANDLE(x) REPN(&AssemblyGeneratorX86::h_##x, WT(x))

	InstructionGenerator AssemblyGeneratorX86::engine[256] = {
		INST_HANDLE(ADD_64)
		INST_HANDLE(ADD_32)
		INST_HANDLE(SUB_64)
		INST_HANDLE(SUB_32)
		INST_HANDLE(MUL_64)
		INST_HANDLE(MULH_64)
		INST_HANDLE(MUL_32)
		INST_HANDLE(IMUL_32)
		INST_HANDLE(IMULH_64)
		INST_HANDLE(DIV_64)
		INST_HANDLE(IDIV_64)
		INST_HANDLE(AND_64)
		INST_HANDLE(AND_32)
		INST_HANDLE(OR_64)
		INST_HANDLE(OR_32)
		INST_HANDLE(XOR_64)
		INST_HANDLE(XOR_32)
		INST_HANDLE(SHL_64)
		INST_HANDLE(SHR_64)
		INST_HANDLE(SAR_64)
		INST_HANDLE(ROL_64)
		INST_HANDLE(ROR_64)
		INST_HANDLE(FPADD)
		INST_HANDLE(FPSUB)
		INST_HANDLE(FPMUL)
		INST_HANDLE(FPDIV)
		INST_HANDLE(FPSQRT)
		INST_HANDLE(FPROUND)
		INST_HANDLE(JUMP)
		INST_HANDLE(CALL)
		INST_HANDLE(RET)
	};
}