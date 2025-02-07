#ifndef SYMBOL_OPTIONS_H
#define SYMBOL_OPTIONS_H

#include <unordered_map>
#include <regex>
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
 * @brief A vector containing information about built-in symbols and constructs.
 *
 * This vector stores predefined symbols and language constructs 
 * that are specified in a JSON file and loaded at runtime.
 */
inline std::vector<Symbol> defaultSymbols;



/**
 * @brief A list of token patterns used for symbol extraction.
 *
 * This unordered map associates different symbol types with their corresponding 
 * regex-based search patterns. Each entry consists of:
 * - A symbol type (`SymbolKind`).
 * - The index of the capture group containing the main identifier (e.g., variable or function name).
 * - The index of the capture group containing the full match with additional details.
 * - A `std::regex` pattern used for matching the symbol in the text.
 *
 * The map is used to identify various C++ constructs such as variables, methods, 
 * functions, structs, and classes.
 */
 
inline std::unordered_map <SymbolKind, SymbolOpt> SymbolOptions =
{ 
    {SymbolKind::Variable,      {3, 0, std::regex(R"(\b([*&]*([a-zA-Z_]+)\s*[*&]*\s+)+[*&]*([a-zA-Z_]+)\s*[;|=])")}},
    {SymbolKind::Method,        {4, 0, std::regex(R"((\w[\w\s*&]+)\s+(\w+)(\s*::\s*)(\w+)\s*\(([^)]*))")}},
    {SymbolKind::Function,      {2, 0, std::regex(R"((\w[\w\s*&]+)\s+(\w+)\s*\(([^)]*)\))")}},
    {SymbolKind::Struct,        {1, 0, std::regex(R"(\bstruct\s+(\w+)\s*)")}},
    {SymbolKind::Class,         {1, 0, std::regex(R"(\bclass\s+(\w+)\s*)")}},
    {SymbolKind::Namespace,     {1, 0, std::regex(R"(\bnamespace\s+(\w+)\s*)")}},
    {SymbolKind::File,          {2, 0, std::regex(R"(#include\s+(")(.+)("))")}},
    {SymbolKind::Package,       {2, 0, std::regex(R"(#include\s+(<)(.+)(>))")}}
};

#endif 
