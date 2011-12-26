/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/** @file
 * \brief This file contains decalration of class NodeTranslator
 */

#pragma once

#include <list>
#include <stdint.h>

// forward declation
class PFLMIRNode;
class CodeList;

/*!
 * \brief This class translates a Codelist of StatemenBase to a std:list<PFLMIRNode*>
 */
class NodeTranslator
{
	private:
		CodeList *_code_list;
		std::list<PFLMIRNode*> *_target_list;
		std::list<PFLMIRNode*> *translate();
		uint32_t _bb_id;
	public: 
		NodeTranslator(CodeList *from, uint32_t bb_id): _code_list(from), _bb_id(bb_id) {
		};

		~NodeTranslator() {};

		void translate(std::list<PFLMIRNode*> *);
};

