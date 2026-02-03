#pragma once
#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <unordered_map>
#include <variant>
#include <string>
#include <string_view>
#include <print>
#include <vector>
#include <type_traits>
#include <optional>
#include <regex>
#include <utility>

namespace bogeygirl {

	struct position {
		size_t index;
		//human readable error messages

		///WARNING: index from 0. first line is 0, second is 1
		size_t line_num;
		size_t char_num;
	
		bool operator==(const position& rhs ) const = default;
	
		void advance(std::string_view complete_input) {
			char c = complete_input[index];
			if (c == '\n') {
				line_num++;
				char_num = 0;	
			} else {
				char_num++;
			}
			index++;
		}
	
		///returns the index of the start of the line
		size_t start_of_line() const {
			return index - char_num;	
		}

		size_t end_of_line(std::string_view complete_input) const {
			size_t eol_index = index;
			while (eol_index < complete_input.size() && complete_input[eol_index] != '\n') {
				++eol_index;
			}
			return eol_index;
		}

		///advance forwards
		//(used to give good numbers in errors) when reparsing
		position offset(position pos) {
			return {pos.index + index, pos.line_num + line_num, pos.char_num + char_num};
		}
	};
}

template <>
struct std::formatter<bogeygirl::position> {
	constexpr auto parse(std::format_parse_context& ctx) {
			return ctx.begin();
	}

	auto format(const bogeygirl::position& pos, std::format_context& ctx) const {
			return std::format_to(ctx.out(), "{}:{} ({})", 
					///adjust from 0 indexed lines; to human expected 1 indexed lines
					pos.line_num+1, pos.char_num, pos.index);
	}
};

namespace bogeygirl {
	
	/// parsing failed, human-readable message and pos is where the error was found 
	struct error_data {
		position pos;
		std::string msg;
		///if we should recover from this error
		///by looking for an alternative parse
		bool deterministic;
		//keeps track of alternatives when trying different options
		size_t alternative = 0;
	};

	struct token {
		///text holds the characters we care about
		///text should be a subset of the range [start_pos.index, end_pos.index)
		///but a string from start_pos.index to end_pos.index can include 
		///garbage at either end
		std::string_view text; 
		std::vector<token> children;
		position start_pos; 
		///one character after the end of the token in the origin string
		///used to signal where the next parse happens next
		position end_pos; 
		size_t alternative = 0;

		//the paramater to the get method
		using get_path = std::vector<size_t>;
		constexpr std::optional<token> get(get_path path) const {
			const token* current_token = this;
			for (const auto& i : path) {
				if (i < current_token->children.size()) {
					current_token = &(current_token->children[i]);
				} else {
					return {};
				}
			}
			return *current_token;
		}

		[[nodiscard]] bool empty() {
			return children.empty() && text.empty();
		}
	
		///returns true if the token holds the invariants we can check
		bool assert_shape() const {
				bool result = true;
				if (start_pos.index > end_pos.index) {
					std::println(stderr, "error malformed token: end_pos {} before start_pos {}",
						end_pos, start_pos);
					result = false;
				}
				if (text.size() > end_pos.index - start_pos.index) {
					std::println(stderr, "Error malformed token \"{}\": size {} != ({} - {}) [{}]",
						text, text.size(), end_pos.index, start_pos.index,
						end_pos.index - start_pos.index);
					result = false;
				};
				return result;
		}

		///tests if the maximum depth is 1
		///i.e. all direct children have no descendant
		///children should be empty of the flattened child
		bool is_flat() const {
			return std::ranges::all_of(children, [](auto x){return x.children.empty();});	
		}
		

	};

	using rule_result = std::variant<error_data, token>;
	void inline pretty_print(const token& t,
		std::string_view indent_str = " ", int indent = 0);

	///the singluar argument passed to each parser
	struct context;
	using skipper_t = std::function<context(context)>;
	struct context {
		std::string_view input;
		skipper_t skipper;
		position pos;

		char operator*() {
			return input[pos.index];
		}

		void operator++() {
			pos.advance(input);
		}

		bool valid() {
			return 0 <= pos.index && pos.index < input.size();
		}

		///advance to a particular index
		context advance(size_t index) const {
			position new_pos = pos; 
			for (size_t i = pos.index; i < index; ++i) {
				new_pos.advance(input);
			}
			return {input, skipper, new_pos};
		}

		///convenience function to shift context by addition
		context operator+(size_t index) const {
			return advance(pos.index + index);
		}

		///advance the context by the skipper
		context skip() const {
			return skipper(*this);
		}

		///advance the context from supplied pos by the skipper
		///equivalent to ctx(pos).skip();
		context skip(size_t pos) const {
			return skipper({input, skipper, advance(pos).pos});
		}

		std::string_view::const_iterator to_iter() const {
			return input.begin() + pos.index;
		}
	};


	template <class F>
	concept RuleAction = std::is_invocable_r_v<rule_result, F, const context&>;

	template<RuleAction T> struct flattening_rule; 
	template<RuleAction T> struct empty_rule; 
	template<RuleAction T> struct rule;

	template<RuleAction T> struct rule {
		T run;
		
		operator empty_rule<T>() const {
			return {run};
		}
		operator flattening_rule<T>() const {
			return {run};
		}
	};

	///A helpper rule to allow R1 >> R2 >> R3 
	///to automatically become [R1, R2, R3]
	///to the parent node.
	///Automatically converts to rule.
	///thus allowing "one named rule per level" style description
	template<RuleAction T> struct flattening_rule {
		T run;

		operator rule<T>() const {
			return {run};
		}
		operator empty_rule<T>() const {
			return {run};
		}
	};

	///rule which returns nothing on parse
	template<RuleAction T> struct empty_rule {
		T run;

		operator rule<T>() const {
			return {run};
		}
		operator flattening_rule<T>() const {
			return {run};
		}
	};


	template<class T> rule(T a) -> rule<T>;
	template<class T> rule(flattening_rule<T> a) -> rule<T>;
	template<class T> rule(empty_rule<T> a) -> rule<T>;
	template<class T> flattening_rule(T a) -> flattening_rule<T>;
	template<class T> flattening_rule(rule<T> a) -> flattening_rule<T>;
	template<class T> flattening_rule(empty_rule<T> a) -> flattening_rule<T>;
	template<class T> empty_rule(T a) -> empty_rule<T>;
	template<class T> empty_rule(rule<T> a) -> empty_rule<T>;
	template<class T> empty_rule(flattening_rule<T> a) -> empty_rule<T>;

	template<class T, template<class> class U>
	concept AnyRule = std::same_as<rule<T>, U<T>>
							 ||   std::same_as<flattening_rule<T>, U<T>> 
							 ||   std::same_as<empty_rule<T>, U<T>>;

	///helper to concatinate to vectors together
	template<class T> std::vector<T>
		concat(const std::vector<T>& a, const std::vector<T>& b) {

		std::vector<T> result;
		result.reserve( a.size() + b.size() );
		result.insert( result.end(), a.begin(), a.end() );
		result.insert( result.end(), b.begin(), b.end() );
		return result;
	}


	/////////////////sequence rules: >> //////////////////////////
	template<RuleAction L, RuleAction R, 
			///method to generate new cities
			std::vector<token> getChildren(const token&, const token&),
			bool deterministic = false,
			///control of the output
			template<class> class RuleType = flattening_rule
		> 
	auto constexpr combine_rules( const rule<L>& lhs, const rule<R>& rhs ) {

		return RuleType 
			{.run = [lhs, rhs](const context& ctx)
			-> rule_result  
			{
				rule_result first_result = lhs.run(ctx);

				if (std::holds_alternative<error_data>(first_result)) {
					//there is a error that we just let pass through us
					return first_result;
				}
				token first_token = std::get<token>(first_result);
				auto [ text_l, children_l, start_l, mid_pos, alternative_l] = first_token;
				

				rule_result second_result = rhs.run(ctx.skip(mid_pos.index));

				if (std::holds_alternative<error_data>(second_result)) {
					//there is a error that we just let pass through us
					error_data error = std::get<error_data>(second_result);
					//promote to a determinsitic error if we are deterministic
					//never demote a determinsitic error
					error.deterministic |= deterministic;
					return error;
				}
				token second_token = std::get<token>(second_result);		
				auto [ text_r, children_r, start_r, end_pos, alternative_r] = second_token; 

				std::vector<token> new_children = getChildren(first_token, second_token); 

				token result = {std::string_view(text_l.begin(),text_r.end()),
												new_children,	
												start_l, end_pos 
												};

				return result;
				}};
	}

	///sequence the two rules
	template<RuleAction L, RuleAction R>
	auto constexpr operator>>( const rule<L>& lhs, const rule<R>& rhs) {
		return combine_rules<L, R, 
			(	[](const token &lt, const token &rt) {return std::vector<token>{lt, rt};} )
			>(lhs, rhs);
	}

	///sequnce and flatten a flatting_rule with a normal rule
	template<RuleAction L, RuleAction R>
	auto constexpr operator>>( const flattening_rule<L>& lhs, const rule<R>& rhs) {
		return combine_rules<L, R, 
			(	[](const token &lt, const token &rt) {
				return concat( lt.children, std::vector({rt}));
			 }) 

			>(lhs, rhs);
	}

	///sequence and flatten two flatting_rules together
	template<RuleAction L, RuleAction R>
	auto constexpr operator>>( const flattening_rule<L>& lhs, const flattening_rule<R>& rhs) {
		return combine_rules<L, R, 
			(	[](const token &lt, const token &rt) {return concat( lt.children, rt.children );} )
			>(lhs, rhs);
	}

	///sequence and flatten a rule with a flattening_rule
	template<RuleAction L, RuleAction R>
	auto constexpr operator>>( const rule<L>& lhs, const flattening_rule<R>& rhs) {
		return combine_rules<L, R, 
			(	[](const token &lt, const token &rt) {return concat( std::vector({lt}), rt.children );} )
			>(lhs, rhs);
	}

	//Ignore empty_type when preparing the result for a sequence
	template<RuleAction L, RuleAction R>
	auto constexpr operator>>( const rule<L> lhs, const empty_rule<R>& rhs) {
		return combine_rules<L, R, 
			[](const token &lt, const token &rt) {return std::vector<token>({lt});}
		>(lhs, rhs);
	}
	template<RuleAction L, RuleAction R>
	auto constexpr operator>>( const flattening_rule<L> lhs, const empty_rule<R>& rhs) {
		return combine_rules<L, R, 
			[](const token &lt, const token &rt) {return lt.children;}
		>(lhs, rhs);
	}
	template<RuleAction L, RuleAction R>
	auto constexpr operator>>( const empty_rule<L>& lhs, const rule<R>& rhs) {
		return combine_rules<L, R, 
			[](const token &lt, const token &rt) {return std::vector<token>({rt});}
		>(lhs, rhs);
	}
	template<RuleAction L, RuleAction R>
	auto constexpr operator>>( const empty_rule<L>& lhs, const flattening_rule<R>& rhs) {
		return combine_rules<L, R, 
			[](const token &lt, const token &rt) {return rt.children;}
		>(lhs, rhs);
	}
	template<RuleAction L, RuleAction R>
	auto constexpr operator>>( const empty_rule<L>& lhs, const empty_rule<R>& rhs) {
		return combine_rules<L, R, 
			[](const token &lt, const token &rt) {return std::vector<token>{};}
		, false, empty_rule
		>(lhs, rhs);
	}
	//////////////sequence without backtracking: > //////////
	template<RuleAction L, RuleAction R>
	auto constexpr operator>( const rule<L>& lhs, const rule<R>& rhs) {
		return combine_rules<L, R, 
			(	[](const token &lt, const token &rt) {return std::vector<token>{lt, rt};} )
			, true>(lhs, rhs);
	}

	template<RuleAction L, RuleAction R>
	auto constexpr operator>( const flattening_rule<L>& lhs, const rule<R>& rhs) {
		return combine_rules<L, R, 
			(	[](const token &lt, const token &rt) {
				return concat( lt.children, std::vector({rt}));
			 }) 

			, true>(lhs, rhs);
	}

	///sequence and flatten two flatting_rules together
	template<RuleAction L, RuleAction R>
	auto constexpr operator>( const flattening_rule<L>& lhs, const flattening_rule<R>& rhs) {
		return combine_rules<L, R, 
			(	[](const token &lt, const token &rt) {return concat( lt.children, rt.children );} )
			, true>(lhs, rhs);
	}

	///sequence and flatten a rule with a flattening_rule
	template<RuleAction L, RuleAction R>
	auto constexpr operator>( const rule<L>& lhs, const flattening_rule<R>& rhs) {
		return combine_rules<L, R, 
			(	[](const token &lt, const token &rt) {return concat( std::vector({lt}), rt.children );} )
			, true>(lhs, rhs);
	}

	//Ignore empty_type when preparing the result for a sequence
	template<RuleAction L, RuleAction R>
	auto constexpr operator>( const rule<L> lhs, const empty_rule<R>& rhs) {
		return combine_rules<L, R, 
			[](const token &lt, const token &rt) {return std::vector<token>({lt});}
		, true>(lhs, rhs);
	}
	template<RuleAction L, RuleAction R>
	auto constexpr operator>( const flattening_rule<L> lhs, const empty_rule<R>& rhs) {
		return combine_rules<L, R, 
			[](const token &lt, const token &rt) {return lt.children;}
		, true>(lhs, rhs);
	}
	template<RuleAction L, RuleAction R>
	auto constexpr operator>( const empty_rule<L>& lhs, const rule<R>& rhs) {
		return combine_rules<L, R, 
			[](const token &lt, const token &rt) {return std::vector<token>({rt});}
		, true>(lhs, rhs);
	}
	template<RuleAction L, RuleAction R>
	auto constexpr operator>( const empty_rule<L>& lhs, const flattening_rule<R>& rhs) {
		return combine_rules<L, R, 
			[](const token &lt, const token &rt) {return rt.children;}
		, true>(lhs, rhs);
	}
	template<RuleAction L, RuleAction R>
	auto constexpr operator>( const empty_rule<L>& lhs, const empty_rule<R>& rhs) {
		return combine_rules<L, R, 
			[](const token &lt, const token &rt) {return std::vector<token>{};}
		, true, empty_rule
		>(lhs, rhs);
	}	

	//////////////alternatives: || //////////
	
	template<class L, class R>
	auto operator||( const L& lhs, const R& rhs) {
		return rule{[lhs, rhs](context ctx) -> rule_result  {
				rule_result first_result = lhs.run(ctx);
				
				if (std::holds_alternative<token>(first_result)) {
					return first_result;
				}
				if (std::get<error_data>(first_result).deterministic) return first_result;

				//try the alternative
				rule_result second_result = rhs.run(ctx);
				if (std::holds_alternative<token>(second_result)) {
					token t = std::get<token>(second_result);
					size_t alternative_level = 1 + std::get<error_data>(first_result).alternative;
					t.alternative += alternative_level;
					return t;
				}

				//we definitely have an error
				error_data l_error = std::get<error_data>(first_result) ;
				error_data r_error = std::get<error_data>(second_result) ;
				return error_data{ctx.pos,
					std::format("{} OR {}", l_error.msg, r_error.msg), r_error.deterministic,
					l_error.alternative + r_error.alternative + 1
					};

		}};
	}

	////////////Permunation: a^b -> a || b || ba || ab ////

	//TODO make this work when chained a ^ b ^ c etc	
	template<class L, class R>
	auto operator^( const L& lhs, const R& rhs) {
		return rule{[lhs, rhs](context ctx) -> rule_result  {
			rule_result result =
				(rule( lhs >> rhs ) || rule( rhs >> lhs )
				|| rule( lhs ) || rule( rhs )).run(ctx);

			if (std::holds_alternative<error_data>(result)) { return result; }


			//TODO think about what t.alternative ought to be
			token& t = std::get<token>(result);
			if (t.alternative == 0) {
				return t;
			}
			if (t.alternative == 1) {
				//flip the order of the children to standardize them for parsers
				token a = t.children.front();
				token b = t.children.back();
				t.children = {b, a};	
				return t;
			}
			//TODO I don't understand what the alternative is doing 
			//std::println("foo {}, {}, {}", t.alternative, t.children.size(), t.text);

			//wrap single results, so they are nested equally to the compount results
			token wrapping_t = t;
			wrapping_t.children = {t};	

			return wrapping_t;
		}};
	}


	////////////Kleene star */////////////////////
	template<RuleAction T, template<class> class R, bool at_least_one = false>
	requires AnyRule<T, R> 
	auto operator*( const R<T> r) {
		return rule{[r](context ctx) -> rule_result {
				position start_pos = ctx.pos;
				std::vector<token> children;	
				error_data last_error;
				while (true) {
					rule_result current_result = r.run(ctx);

					if (std::holds_alternative<error_data>(current_result)
								or ctx.pos.index > ctx.input.length()) {
						//Big error; don't try to recover
						if (std::get<error_data>(current_result).deterministic) {
							return current_result;
						}
						//there is a error that we stop looking for more;
						//but it isn't an error
						last_error = std::get<error_data>(current_result);
						break;
					}
					token current_token = std::get<token>(current_result);
					ctx = ctx.skip(current_token.end_pos.index);
					
					//avoid extra nesting layers
					//in the case of flattening_rules
					if constexpr (std::same_as<R<T>, rule<T>>) {
						children.push_back(current_token);
					} else if constexpr (std::same_as<R<T>, flattening_rule<T>>) {
						children.insert( children.end(),
							current_token.children.begin(), current_token.children.end() );
					} else if constexpr (std::same_as<R<T>, empty_rule<T>>) {
						children.push_back(current_token);
					} else {
						std::unreachable();
					}
				}
				
				if (children.empty()) {
					if constexpr (at_least_one) {
						return error_data{last_error.pos, 
							std::format("didn't find even one, because {}",
								 last_error.msg), false};
					} else {
							
							return token{std::string_view(ctx.input.begin() + ctx.pos.index, 
																					ctx.input.begin() + ctx.pos.index),

													{}, ctx.pos, ctx.pos};
					}
				}
				return token{std::string_view(ctx.input.begin() + start_pos.index, 
				                              ctx.input.begin() + ctx.pos.index),
							       children,
				             start_pos,
										 ctx.pos
										};
		}};
	}

	//////////////at least 1 + /////////
	template<RuleAction T, template<class> class R>
	requires AnyRule<T, R> 
	auto operator+( const R<T> r) {
		return operator*<T, R, true>(r);
	}

	///////////////Optional -/////////////////////
	template<RuleAction T, template<class> class R>
	requires AnyRule<T, R> 
	auto operator-( const R<T> r) {
		return R{[r](context ctx) -> rule_result {
				std::vector<token> children;	
				
				rule_result current_result = r.run(ctx);

				if (std::holds_alternative<error_data>(current_result)) {
					if (std::get<error_data>(current_result).deterministic) {
						return current_result;
					} else {
						//didn't find the optional thing, so 
						return token{std::string_view(ctx.input.begin() + ctx.pos.index, 
																					ctx.input.begin() + ctx.pos.index),

									{}, ctx.pos, ctx.pos	
									};
					}
				}
				token current_token = std::get<token>(current_result);
				children.push_back(current_token);
				
				return token{current_token.text,
								{current_token}, 
								current_token.start_pos, current_token.end_pos 
								
								};
		}};
	}

	/////////////////Don't Match -///////////
	template<RuleAction L, template<class> class LT, 
					RuleAction R, template<class> class RT>
	requires AnyRule<L, LT> && AnyRule<R, RT> 
	auto constexpr operator-( const LT<L>& lhs, const RT<R>& rhs) {
		return LT{[lhs, rhs](context ctx) -> rule_result {
				std::vector<token> children;	
				
				rule_result primary_result = lhs.run(ctx);

				if (std::holds_alternative<error_data>(primary_result)) {
					return primary_result;
				}

				rule_result not_result = rhs.run(ctx);
				if (std::holds_alternative<error_data>(not_result)) {
					return primary_result;
				}
				
				token evil_match = std::get<token>(not_result);	
				return error_data{ evil_match.start_pos,
							std::format("expected not to see \"{}\"", evil_match.text),
							false};

		}};
	}

	template<RuleAction L, template<class> class LT, class F> 
	requires (AnyRule<L, LT> && requires (token t){
			F(t);
	})
	auto constexpr applyFunc( const LT<L>& lhs, F&& func) {
		return rule{[lhs, func](context ctx) -> rule_result {
				rule_result naive_result = lhs.run(ctx);

				if (std::holds_alternative<error_data>(naive_result)) {
					return naive_result;
				}	else {
					return func(std::get<token>(naive_result));
				}
		}};
	}

	//this is potentially inefficient (lots of copying)
	inline token flatten_token(token t) {
		//iterator over the tree gathering up children
		std::vector<token> result;
		std::stack<token, std::vector<token>> stack; //used for recursion
		stack.push(t);
		while (!stack.empty()) { //our recursion
			auto top = stack.top();
			stack.pop();	
			if (top.alternative != 0) {
				std::println(stderr, "losing alternative information by flattening '{}'", top.text);
			}
			if (top.children.empty() && !top.text.empty()) {
				result.push_back(std::move(top));
			}
			else stack.push_range(std::ranges::views::reverse(top.children));
		}
		t.children = result ;
		return t;
	}
	///rule to move all children to directly under us
	template<RuleAction L, template<class> class LT> 
	requires AnyRule<L, LT>
	auto constexpr flatten( const LT<L>& lhs) {
		return applyFunc(lhs, flatten_token);
	}
	

	///squeeze mutates token by removing all tokens that only have one child, moving an alternative, to their solo children. 
	inline token squeeze_token(token& t) {
		//iterator over the tree
		std::stack<std::reference_wrapper<token>> stack; //used for recursion
		stack.push(t);
		while (!stack.empty()) { //our recursion
			token& top = stack.top();
			stack.pop();	
			
			if (top.children.size() == 1)	{
				top.children.front().alternative = top.alternative;
				top = top.children.front();
			}
			else stack.push_range(std::ranges::views::reverse(top.children));
		}
		return t;
	}

	///rule to move all children to directly under us
	template<RuleAction L, template<class> class LT> 
	requires AnyRule<L, LT>
	auto constexpr squeeze( const LT<L>& lhs) {
		return applyFunc(lhs, squeeze_token);
	}

	///copy alternative values to our direct children	
	inline token& spread_token_alternative_to_children(token& t) {
		for (auto& i : t.children) {
			if (i.alternative != 0) {
				std::println("warning: copying down alternative ({}) is erasing alternative ({}) info on {} ({})", t.alternative, i.alternative, i.text, i.start_pos);
		}
			i.alternative = t.alternative;
		}	
		return t;
	}

	///rule to move all children to directly under us
	template<RuleAction L, template<class> class LT> 
	requires AnyRule<L, LT>
	auto constexpr copy_alt_down( const LT<L>& lhs) {
		return applyFunc(lhs, spread_token_alternative_to_children);
	}

	/// separated list
	/// (a % lit(',') ) means a comma separated list
	template<RuleAction L, template<class> class LT, 
					RuleAction R, template<class> class RT>
	requires AnyRule<L, LT> && AnyRule<R, RT> 
	auto constexpr operator%( const LT<L>& lhs, const RT<R>& rhs) {
		return LT{[lhs, rhs](context ctx) -> rule_result {
						rule the_rule = lhs >> *(rhs  >> lhs);
						rule_result naive_result = the_rule.run(ctx);
						if (std::holds_alternative<error_data>(naive_result)) {
							return naive_result;
						}	else {
							token& t = std::get<token>(naive_result);
							if (t.children.size() == 2) {
								//we want to pull the list in the second
								//arguments to the top level of children
								token the_list = std::move(t.children.at(1));
								t.children.pop_back();		
								t.children.append_range(std::move(the_list.children));
								return t;
							}
							return error_data{ctx.pos, 
									std::format(
										"internal parser error:"
										"there should only be two children,"
										"but there are {}", t.children.size()), true};
						}
					}};
	}


	template<RuleAction L, template<class> class LT> 
	requires AnyRule<L, LT>
	auto constexpr unnest( const LT<L>& lhs) {
		return rule{[lhs](context ctx) -> rule_result {
				rule_result naive_result = lhs.run(ctx);

				if (std::holds_alternative<error_data>(naive_result)) {
					return naive_result;
				}

				token t = std::get<token>(naive_result);
				if (t.children.size() != 1) {
							return error_data{ctx.pos, 
									std::format(
										"internal parse error: "
										"bogeygirl::unest requires there only be 1 child "
										"but there are {}", t.children.size()), true};
				}
				
				//we stitch together a new token containing just the contents
				const token& tt = t.children.front();
				t.text = tt.text; 
				t.children = std::move(tt.children); 
				return t;
		}};
	}

	//////////////convert skippers to rules//
	///convert skippers to rules
	template<class T>
	auto constexpr by_skip(T skipper) {
		return empty_rule{[skipper](context ctx) -> rule_result {
			auto& [input, _, start_pos] = ctx;
			auto [input2, __, end_pos] = skipper(ctx);

			return token({
				std::string_view{
				 input.begin() + start_pos.index,
				 input.begin() + end_pos.index
				},
				{}, start_pos, end_pos});
			
		}};
	}

	///convert rules to skippers (untested)
	template<RuleAction T, template<class> class Rule>
	requires AnyRule<T, Rule>
	skipper_t constexpr to_skip(Rule<T> r, skipper_t skipper) {
		return [r, skipper](context ctx) {
			skipper_t original_skipper = ctx.skipper;
			ctx.skipper = skipper;	
			rule_result result = r.run(ctx);
			if (std::holds_alternative<error_data>(result)) {
			} else {
				ctx = ctx.advance(std::get<token>(result).end_pos.index).skip();
			}
			ctx.skipper = original_skipper;
			return ctx;	
		};
	};

	//////////////pre-made tokens/////////////

	inline bool valid_word(char c) {
		return ('A' <= c && c <= 'Z') || 
					(	'a' <= c && c <= 'z') || 
					(	'0' <= c && c <= '9') || 
					(	'_' == c);
	}

	inline bool valid_digits(char c) {
		return (	'0' <= c && c <= '9'); 
	}

	inline bool valid_alpha(char c) {
		return ('A' <= c && c <= 'Z') || 
					(	'a' <= c && c <= 'z');
	}

	inline bool valid_whitespace(char c) {
		return ' ' == c || 
					 '\n' == c ||
		       '\r' == c ||
           '\t' == c;
	}

	template<bool F(char)>
	rule_result word_def(context ctx) {
		//there was no word character to consume
		position start = ctx.pos;
		if (!ctx.valid()) return error_data{ctx.pos, "ran out of input", false};
		if (!valid_word(*ctx)) {
				return error_data{ctx.pos, "no word character found", false};
		}
		const std::string_view input = ctx.input;
		while (ctx.valid() && F(*ctx)) {
			++ctx;
		}
		return token{input.substr(start.index, ctx.pos.index - start.index), 
					{}, start, ctx.pos};
	}

	constexpr rule word = {word_def<valid_word>};
	constexpr rule alpha = {word_def<valid_word>};
	constexpr rule digits = {word_def<valid_digits>};

	struct literal_generator {
		std::string_view word;
		rule_result operator()(context ctx) const {
			auto& [input, skipper, pos] = ctx;
			position start_pos = pos;
			size_t& index = pos.index;
			size_t offset = 0;
			while ( offset < word.size()) {
				if (index >= input.size()) {
					return error_data({ctx.pos,
						std::format("ran out of input, expected '{}' from '{}'", 
								word[offset], word) 
						, false});
				}
				if (word[offset] != input[index]) {
					return error_data({ctx.pos,
						std::format("unexpected '{}', expected '{}' from '{}'",
							 input[index], word[offset], word)
						, false});
				}

				++offset;
				++ctx;
			}

			return token({input.substr(start_pos.index, offset),
											{}, start_pos, pos});
		}
	};

	///rule that matches the passed string only makes an empty rule
	inline auto constexpr lit(std::string_view word) {
		auto r = literal_generator(word);
		return empty_rule<decltype(r)>(r);
	}

	struct regex_generator {
		std::regex pattern{};
		std::string_view name{};

		rule_result operator()(context ctx) const {
				
			std::match_results<std::string_view::iterator> match;
			if (ctx.valid() && 
					std::regex_search(ctx.input.begin() + ctx.pos.index, ctx.input.end(),
					match, pattern)) {
					std::sub_match sub = match[0];
					token t = token{{sub.first, sub.second}, {},
							ctx.pos, (ctx + (size_t) sub.length()).pos};
					return t;
			}

			return error_data{ctx.pos, std::format("expected '{}'", name), false};
		}
	}; 


	///match a regular expression (no children, but makes a normal rule)
	inline constexpr auto rex(std::string_view pattern, std::string_view name) {
		std::string pattern2 = "^" + std::string(pattern);	
		auto r = regex_generator{std::regex(pattern2), name};
		return rule<decltype(r)>(r);
	}

	inline constexpr auto rex(std::string_view pattern) {
		return rex(pattern, pattern);
	}

	inline const rule float_ = rex("[+-]?[0-9]+(\\.[0-9]+)?", "floating point number");
	inline const rule int_ = rex("[+-]?[0-9]+", "integer");

	//permitted characters
	//but the paired characters must be balanced
	struct banned_balanced_generator {
		std::string_view banned = "uninited";
		std::string_view pairs = "{}[]()";

		rule_result operator()(context ctx) const {
			std::unordered_map<char, int> count{};
			context old_ctx = ctx;

			while (true) {
				char ch = *ctx;
				if (banned.contains(ch)) { break;};
				auto index = pairs.find(ch);
				//simple premitted character so we can skip
				if (index != std::string_view::npos) {
					if (index % 2 == 0) {
						//openning a paired character
						++count[ch];
					} else {
						//we use the left part of the pair as the index
						char left_ch = pairs[index-1];
						//closening a paired character
						--count[left_ch];
						//we closed everything we openned so we're done
						if (count[left_ch] < 0) {
							break;
						};
					}
				}
				++ctx;
				if (!ctx.valid()) { std::println("warning: 'balanced-banned-generator' exited through EOS"); break;} ;
			}

			//check balanced ness of pairs
			for (const auto& [key,value] : count) {
				if (value != 0 && value != -1) {
					std::println("{} wasn't balanced by the time it ends, it is {}, starting at {} and ending at {}", key, value, old_ctx.pos, ctx.pos);
					return error_data{ctx.pos,
						std::format("{} wasn't balanced by the time it ends, it is {}", key, value),
					false};
				}
			}
		
			if (old_ctx.pos.index == ctx.pos.index) {
				return error_data{ctx.pos,
					std::format(
						"expected at least 1 characted besides {} or a closing character", banned),
					false};
			}	
	
			return token({{old_ctx.to_iter(), ctx.to_iter()},
											{}, 
										old_ctx.pos, ctx.pos});
		}
	}; 

	///matches lots of characters, but requires brackets
	///are balanced (doesn't require proper nesting though)
	inline constexpr auto balanced_everything_but(std::string_view banned_chars) {
		auto r = banned_balanced_generator{banned_chars};
		return rule<decltype(r)>(r);
	}

	/// skip spaces and tabs
	inline context space_skip(context ctx) {
		const std::string_view input = ctx.input;
		size_t &pos = ctx.pos.index;
		while (pos < input.size() && ('\t' == input[pos] || ' ' == input[pos])) {
			 ++ctx;
		}
		return ctx;
	} 

	/// skips all white space
	inline context whitespace_skip(context ctx) {
		const std::string_view input = ctx.input;
		size_t &pos = ctx.pos.index;
		while (ctx.valid() && valid_whitespace(input[pos])) {
			 ++ctx;
		}
		return ctx;
	}

	const inline bogeygirl::empty_rule newln =
		bogeygirl::by_skip(bogeygirl::space_skip) >> bogeygirl::lit("\n") >>
		bogeygirl::by_skip(bogeygirl::whitespace_skip);

	inline std::string repeat(std::string_view str, int times) {
		std::string result;
		result.reserve(str.size() * times);
		for (int i = 0; i < times; ++i) {
			result.append(str);
		}
		return result;
	}

	inline void print_error(std::string_view input, error_data e, 
			position offset = {0,0,0} //used for reparsing
			) {
			std::println(stderr, "Parsing Error{} {} at {}",
				e.deterministic ? "!" : ":" , e.msg, e.pos.offset(offset));

			size_t line_start = e.pos.start_of_line();		
			size_t line_end = e.pos.end_of_line(input);	

			constexpr size_t context_size = 5;

			size_t forwards_pos = std::min(e.pos.index + context_size, line_end);
			size_t backwards_context_size = std::min( e.pos.index - line_start, context_size );
			size_t backwards_pos =	e.pos.index - backwards_context_size; 
			std::println(stderr, "{}",
				input.substr(backwards_pos, forwards_pos-backwards_pos));
			std::println(stderr, "{}", repeat("-", backwards_context_size)+"^ here");
	}

	template<RuleAction T>
	[[nodiscard]]
	std::optional<token> prase(
		const rule<T> r, //the rule which describes the entire structure
		skipper_t skip, //skipping rule
		const std::string_view input, //input to parse
		const position offset_amount = {0,0,0} //set to make errors correct when reparsing a substring
		){

		context ctx = {input, skip, {0,0,0}};
		rule_result result = r.run(ctx);

		if (std::holds_alternative<error_data>( result )) {
			print_error(input, std::get<error_data>(result), offset_amount);
			return {};
		} 
		token tree =  std::get<token>( result );

		if (tree.end_pos.index == input.size() ||
				//ignore a newline at the very end of the stream
				(tree.end_pos.index == input.size() - 1 && input.back() == '\n')) {
			return tree;
		}

		print_error(input, error_data{tree.end_pos, "unknown garbage at end of parse"},
			offset_amount);
		return {};
	}

	void inline print_indent(int level, std::string_view indent_str) {
		for (int i = 0; i < level; i++) {
			std::print("{}", indent_str);
		}	
	}

	void inline pretty_print(const token& t,
		std::string_view indent_str, int indent) {
			t.assert_shape(); //print warnings if a malformed token was made
			print_indent(indent, indent_str);
			if (t.text.empty()) {
				std::print("{{}}");
			} else if (t.children.empty()) {
				std::print("{}", t.text);
			} else {
				std::println("{{ : {} children", t.children.size());
				for (const auto& child : t.children) {
					pretty_print(child, indent_str, indent + 1);
				}
				print_indent(indent, indent_str);
				std::print("}}", t.end_pos);
		 }
		 if (t.alternative != 0 ) std::print(" alt:{}", t.alternative); //view alternative info
		 std::println();
	}
}

template <>
struct std::formatter<bogeygirl::token::get_path> {
	constexpr auto parse(std::format_parse_context& ctx) {
			return ctx.begin();
	}

	auto format(const bogeygirl::token::get_path path, std::format_context& ctx) const {
			auto result = ctx.out();
			for (const auto& i : path) {
				result = std::format_to(result, "-{}-", i);
			}

			return result;
	}
};

// vim: ts=2 sw=2
