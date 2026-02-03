#pragma once
#include "bogeygirl/bogeygirl.hpp"
namespace parsing_bits {
	const static bogeygirl::empty_rule newln_or_c = 
		+(
		bogeygirl::by_skip(bogeygirl::space_skip) >> 
		bogeygirl::empty_rule(-bogeygirl::rex("#.*", "a comment"))	>>
		bogeygirl::lit("\n") >>
		bogeygirl::by_skip(bogeygirl::whitespace_skip)
	);
	const static bogeygirl::rule most_anything = bogeygirl::rex("[^[:space:]=#[\\]]+", "most anything");
	const static bogeygirl::rule anything = bogeygirl::rex("[^#\\n]+", "anything");
	const static bogeygirl::rule pair =  most_anything >
		bogeygirl::lit("=") > anything > newln_or_c;
}
// vim: ts=2 sw=2
