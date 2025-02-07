#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include "symbolOptions.hpp"


/**
 * @brief Extracts a block of code enclosed in curly braces ({}) from the given text.
 *
 * This function takes iterators pointing to the beginning and the end of the text,
 * searches for the matching pair of curly braces, and returns an iterator to the
 * end of the block (i.e., the closing brace). It throws an exception if no valid
 * pair of curly braces is found.
 *
 * @param begin Iterator pointing to the start of the block (opening brace).
 * @param end Iterator pointing to the end of the text where the search stops.
 * @return std::string::const_iterator Iterator pointing to the closing brace of the block.
 * @throws std::invalid_argument If no matching pair of curly braces is found.
 */
 
std::string::const_iterator extractBlock(const std::string::const_iterator begin, const std::string::const_iterator end);

void ignoreComment(std::string& text);

void findLibSymbols(std::string& text);


/**
 * @brief Searches for symbols in the given text.
 *
 * This function searches for symbols within the specified text using a provided 
 * regular expression. It takes the range of text (begin and end iterators) to search, 
 * the type of symbol to look for, and a vector where found symbols are added.
 * It may be called recursively to search for symbols within class definitions or 
 * local variables in function bodies, where the range of text is narrowed to the 
 * corresponding block (e.g., class definition or function body).
 * 
 * @param text The text in which to search for symbols.
 * @param begin Iterator pointing to the beginning of the search range.
 * @param end Iterator pointing to the end of the search range.
 * @param regex The regular expression used to search for symbols.
 * @param kind The type of symbol to search for (e.g., variable, function, class).
 * @param symbolList A vector where the found symbols will be stored.
 * 
 * @note This function may be called recursively to search for fields of classes or 
 * local variables in functions. When searching within such blocks, the `begin` 
 * and `end` iterators should point to the respective block (class definition or 
 * function body). For adding child symbols, the `Symbol::children` field should be used.
 */
 
void symbolSearch(std::string& text,
                    std::string::const_iterator begin,
                        std::string::const_iterator end,
                            std::regex& regex,
                                SymbolKind kind,
                                    std::vector <struct Symbol>& symbolList);
