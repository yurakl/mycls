
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include "json.hpp"
#include "client.hpp"  
#include <regex>
#include "textProcessor.hpp"
#include "symbolOptions.hpp"

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


 
