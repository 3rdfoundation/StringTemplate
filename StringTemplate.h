#pragma once
class StringTemplate
{
public:

	StringTemplate();
	~StringTemplate();

	std::string applyReplacements(
		const std::string& str, const std::map<std::string, std::string>& replacements);

private:

	std::vector<std::string> getSegments(const std::string& str);

	size_t getBracketEndPosition(
		const std::string& str, const size_t& startingPos, const std::string& endBracket, const std::string& startBracket);

	std::string processSegments(
		const std::vector<std::string>& segments, const std::map<std::string, std::string>& replacements);

	std::string StringTemplate::processSegment(
		const std::string& segment, const std::map<std::string, std::string>& replacements, const int& iteration);

	std::string StringTemplate::getConditionalKey(const std::string& segment);

	std::string processConditionalSegment(
		const std::string& segment, const std::map<std::string, std::string>& replacements, const std::string& key);

	std::string StringTemplate::replaceAllOccurrences(
		const std::string& segment, const std::string& oldSubstring, const std::string& newSubstring);
};