
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <utility>
#include "json.hpp"
#include "client.hpp"  
#include <regex>  
using json = nlohmann::json;

/**
 * 
 * @brief Processes all incoming HTTP requests.
 *
 * This function extracts HTTP requests from the given raw input string
 * and processes them, storing responses in the provided queue.
 *
 * @param request Raw request extracted from a pipe. It may contain a partial or multiple HTTP requests.
 * @param answer_queque A queue where processed responses are stored.
 * @return void
 * 
 */
void processAllRequests(std::string& request, std::vector<std::string>& answer_queque);

/**
 * @brief Processes a single JSON request and calls the appropriate handler.
 *
 * This function takes a JSON request as input, parses it, and determines
 * the appropriate handler to process the request.
 *
 * @param request JSON-formatted string containing the request data.
 * @param answer_queue A queue where the response from the handler will be stored.
 * @return void
 *
 * @callgraph
 * @callergraph
 */
int  processLSPRequest(const std::string& request, std::string& answer);

/**
 * @brief Handles the "initialize" request.
 *
 * This function is called when the "initialize" request is received. 
 * It processes the input JSON data and sets the appropriate response.
 *
 * @param j The input JSON data associated with the "initialize" request.
 * @param answer The string where the response will be stored.
 *
 * @callgraph
 * @callergraph
 */
void handleInitialize(const json& j, std::string& answer);

/**
 * @brief Handles changes to the text.
 *
 * This function is called when the text is modified. It takes the new text 
 * as input and replaces the existing content in the buffer, which stores 
 * the text of the corresponding file.
 *
 * @param j The new text (in JSON format) to replace the existing content in the buffer.
 */
void onDidChange(const json& j);

/**
 * @brief Handles file opening events.
 *
 * This function is triggered when a file is opened. It adds the file to the project, 
 * creates a buffer for it, and copies the file's content into the buffer. 
 * No response is returned by this function.
 *
 * @param j The input JSON data associated with the file opening event.
 * @param answer The string where a response would be stored (not used in this function).
 */
void onDidOpen(const json& j, std::string& answer);

/**
 * @brief Handles file closure events.
 *
 * This function is triggered when a file is closed. It does not generate a response.
 * If the closed file is included in another open file (via `#include` or similar),
 * the program keeps the file "open" to avoid closing it prematurely.
 *
 * @param j The input JSON data associated with the file closure event.
 * @param answer The string where a response would be stored (not used in this function).
 */
void onDidClose(const json& j, std::string& answer);

/**
 * @brief Searches for symbols and generates a response.
 *
 * This function performs a search for symbols based on the provided data. After completing 
 * the search, it analyzes the `symbolList` and generates a JSON response containing the 
 * symbols found. It also includes any child elements associated with the symbols, 
 * if applicable.
 *
 * @param j The input JSON data containing the request for symbol search.
 * @param answer The string where the JSON response will be stored.
 */
void onDocumentSymbol(const json& j, std::string& answer);

/**
 * @brief Provides name suggestions for code completion.
 *
 * This function performs a search for symbols in both the built-in symbols (`defaultSymbols`) 
 * and user-defined symbols (`symbolList`). When a position is reached that corresponds to 
 * accessing fields of an object (e.g., `object.` or `object->`), it returns the names of 
 * the fields of the corresponding class or structure. If the position corresponds to 
 * accessing a namespace (or a static class member, such as `Name::`), it returns the 
 * fields of the respective container.
 *
 * @param j The input JSON data containing the context for the completion request.
 * @param answer The string where the completion suggestions will be stored.
 */
void onComletion(const json& j, std::string& answer);

/**
 * @brief Structure containing information for symbol search.
 *
 * This structure holds the necessary data for searching symbols using regular expressions. 
 * It includes the indices of the matched name and detail, as well as the regular expression 
 * used for the search.
 */
struct SymbolOpt { 
    /**
     * @brief Index of the match[] array where the symbol's name is located after 
     * applying the regular expression search.
     */
    int name;

    /**
     * @brief Index of the match[] array where the detailed information about the symbol 
     * is located after applying the regular expression search.
     */
    int detail;

    /**
     * @brief The regular expression used to search for the symbol.
     */
    std::regex regex;
};

enum class SymbolKind {
    File = 1,
    Module = 2,
    Namespace = 3,
    Package = 4,
    Class = 5,
    Method = 6,
    Property = 7,
    Field = 8,
    Constructor = 9,
    Enum = 10,
    Interface = 11,
    Function = 12,
    Variable = 13,
    Constant = 14,
    String = 15,
    Number = 16,
    Boolean = 17,
    Array = 18,
    Object = 19,
    Key = 20,
    Null = 21,
    EnumMember = 22,
    Struct = 23,
    Event = 24,
    Operator = 25,
    TypeParameter = 26
};


/**
 * @brief A unified descriptor for a symbol.
 *
 * This structure is used to represent a symbol in the code. It contains various 
 * properties that describe the symbol, including its name, type, documentation, 
 * insertion text, and the lines where the symbol is found. It also supports 
 * hierarchical relationships, allowing for child symbols, such as fields of a class 
 * or members of a function.
 *
 * @note The `type` is only used for user-defined types and helps in identifying 
 * the object type when invoking symbol completion or searching for field names. 
 * It is not used for built-in types.
 */
struct Symbol {
    /**
     * @brief The name of the symbol.
     */
    std::string label;

    /**
     * @brief A description of the symbol. This is filled when a user-defined symbol is found.
     */
    std::string detail;

    /**
     * @brief The type of the symbol (e.g., variable, function, class).
     */
    SymbolKind kind;

    /**
     * @brief The type of the user-defined object. Used for symbol completion or 
     * searching for field names. Not used for built-in types.
     */
    std::string type;

    /**
     * @brief A detailed description of the symbol, used for built-in types.
     */
    std::string documentation;

    /**
     * @brief The text to insert for this symbol, used for built-in constructs.
     */
    std::string insertText;

    /**
     * @brief The format of the insertion text (e.g., plain text, snippet).
     */
    int insertTextFormat;

    /**
     * @brief The line number where the symbol starts in the code.
     */
    int startLine;

    /**
     * @brief The line number where the symbol ends in the code.
     */
    int endLine;

    /**
     * @brief A list of child symbols. Used for user-defined data (e.g., fields of classes, 
     * members of functions, etc.).
     */
    std::vector<Symbol> children;
};

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

std::string::const_iterator extractBlock(const std::string::const_iterator begin, const std::string::const_iterator end);

void findLibSymbols(std::string& text);
