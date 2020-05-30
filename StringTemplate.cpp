#include "cpputil.h"
#include "StringTemplate.h"

StringTemplate::StringTemplate() {
}

StringTemplate::~StringTemplate() {
}

// ----------------------------------------------------------------------------
// SUPPORTED:
// > static.....: -=<KEY>=-
// > conditional: -={KEY:text -=<KEY>=- }=-
// > repeating..: -={KEY_999:text -={KEY2_999}=- }=- : values start with 1 not 0
//
// NOT SUPPORTED (UNTIL NEEDED):
// > multiple keys: -={KEY,KEY2:text}=-
// ----------------------------------------------------------------------------
// non-error test scenarios:
//  string with no replacments
//  -=<KEY>=- where KEY is in map
//  -=<KEY>=- where KEY is not in map (gets nothing)
//  -={KEY:}=- no text within conditional replacment
//  -={KEY:text}=- plain text (KEY exists, KEY doesn't exist)
//  -={KEY:-=<KEY>=- -=<KEY2>=-}=- multiple KEYs within
//  -={KEY_999:text}=- plain text (KEY_1 exists, KEY_1 & KEY_2 exist, KEY_1 doesn't exist)
//  -={KEY_999:-=<KEY_999>=- -=<KEY2_999>=- -=<KEY>=-}=- plain text (KEY & KEY_1 & KEY2_1 exist, KEY_1,KEY_2 & KEY2_1 exists)
// ----------------------------------------------------------------------------
// error test scenarios:
//  string ends with -=<, -={
//  non replacement -=[
//  unmatched -=<, -={
//  no colon after -={
//  no key after -={
//  -={ within segment (unsupported recursion detected)
//  -=< or -={ within key
// ----------------------------------------------------------------------------
std::string StringTemplate::applyReplacements(
	const std::string& str, const std::map<std::string, std::string>& replacements) {

	std::vector<std::string> segments = getSegments(str);
	return processSegments(segments, replacements);
}

// This gets segments including the brackets surrounding them.
//  Processing the segments occurs in other code (which could eventually be recursive)
//
// Example: 
//  my text -=<ONE>=- more text -={TWO:-=<TWO>=-}=- final text
//
// Resulting Segments:
//  0 = "my text "
//  1 = "-=<ONE>=-"
//  2 = " more text "
//  3 = "-={TWO:-=<TWO>=-}=-"
//  4 = " final text"
// 
std::vector<std::string> StringTemplate::getSegments(const std::string& str) {

	std::vector<std::string> segments;
	
	size_t segmentStartPos = 0;
	int safetyValve = 0;
	while (safetyValve < 100) {
		safetyValve++;

		// Find the next bracket for each type
		size_t staticBracketStartPos = str.find("-=<", segmentStartPos + 1);
		size_t conditionalBracketStartPos = str.find("-={", segmentStartPos + 1);
		if (staticBracketStartPos == std::string::npos && conditionalBracketStartPos == std::string::npos) {
			break;
		}

		// Negate the search for the later bracket if both are found
		if (staticBracketStartPos != std::string::npos && conditionalBracketStartPos != std::string::npos) {
			if (staticBracketStartPos < conditionalBracketStartPos) {
				conditionalBracketStartPos = std::string::npos;
			}
			else {
				staticBracketStartPos = std::string::npos;
			}
		}
		
		// Get the correct end bracket
		size_t bracketStartPos;
		std::string startBracket;
		std::string endBracket;
		if (staticBracketStartPos != std::string::npos) {
			bracketStartPos = staticBracketStartPos;
			startBracket = "-=<";
			endBracket = ">=-";
		}
		else {
			bracketStartPos = conditionalBracketStartPos;
			startBracket = "-={";
			endBracket = "}=-";
		}

		// non replacement text prior to bracket starting position
		if (bracketStartPos > segmentStartPos) {
			segments.push_back(str.substr(segmentStartPos, bracketStartPos - segmentStartPos));
			segmentStartPos = bracketStartPos;
		}

		// Find the bracket end pos
		// -={KEY: text }=-
		// -=<YE_FUN_KEY>=-
		// ^              ^
		// |              |
		// pos         end pos
		size_t bracketEndPos = getBracketEndPosition(str, bracketStartPos, endBracket, startBracket);

		if (bracketEndPos == std::string::npos) {
			// Couldn't find the end bracket ... template is faulty and we are done
			break;
		}

		// Add the bracket replacment segment
		segments.push_back(str.substr(segmentStartPos, bracketEndPos - segmentStartPos + 1));
		segmentStartPos = bracketEndPos + 1;

	}

	// Add last segment (if there is one)
	if (segmentStartPos < str.size() - 1) {
		segments.push_back(str.substr(segmentStartPos, str.size() - segmentStartPos));
	}

	return segments;
}

// This method returns the end of the bracket.
// > If not found it returns the natural std::string::npos value indicating not found
//
// This method can be enhanced for recursive end bracket "skipping":
//
// text -={KEY: text -={K:x}=- text }=-
//      ^            ^       ^        ^
//      |            |       |        |
//  startingPos      recursion     returned
//
size_t StringTemplate::getBracketEndPosition(
	const std::string& str, const size_t& startingPos, const std::string& endBracket, const std::string& startBracket) {
	
	size_t endBracketPos;

	int outstandingBrackets = 1;
	int searchStart = startingPos + 1;
	while (outstandingBrackets > 0) {

		// look for the end bracket
		endBracketPos = str.find(endBracket, searchStart);

		// missing end bracket detection (template error)
		if (endBracketPos == std::string::npos) {
			break;
		}

		// nested / bracket detection
		size_t startBracketPos = str.find(startBracket, searchStart);
		if (startBracketPos != std::string::npos && startBracketPos < endBracketPos) {
			outstandingBrackets++;
			searchStart = startBracketPos + 1;
			continue;
		}

		// end bracket match valid
		searchStart = endBracketPos + 1;
		outstandingBrackets--;
	}

	// return the position of interest
	if (endBracketPos != std::string::npos) {
		endBracketPos += 2;
	}

	return endBracketPos;
}

std::string StringTemplate::processSegments(
	const std::vector<std::string>& segments, const std::map<std::string, std::string>& replacements) {

	std::stringstream ss;
	for (std::string segment : segments) {
		ss << processSegment(segment, replacements, 0);
	}

	return ss.str();
}

std::string StringTemplate::processSegment(
	const std::string& segment, const std::map<std::string, std::string>& replacements, const int& iteration) {

	std::string value;
	if (segment.size() < 7) {
		value = segment;
	}
	// static replacement
	else if (segment.substr(0, 3) == "-=<") {
		if (segment.substr(segment.size() - 3, 3) == ">=-") {
			std::string key = segment.substr(3, segment.size() - 6);

			// if the key ends with _999, replace with 999 the iteration
			if (key.rfind("_999") == (key.size() - 4)) {
				key = key.substr(0, key.size() - 3) + std::to_string(iteration);
			}

			std::string val;
			try {
				val = replacements.at(key);
			}
			catch (const std::out_of_range&) {
				val = ""; // "[STATIC NO VALUE:" + segment + "(KEY=" + key + ")]";
			}
			value = val; // "[STATIC:" + segment + "(KEY=" + key + ",VAL=" + val + ")]";
		}
		else {
			value = "[STATIC WITH NO CLOSE:" + segment + "(" + segment.substr(segment.size() - 3, 3) + ")]";
		}
	}
	// conditional replacement
	else if (segment.substr(0, 3) == "-={") {
		if (segment.substr(segment.size() - 3, 3) == "}=-") {
			std::string key = getConditionalKey(segment);
			value = "[CONDITIONAL:" + segment + "(KEY=" + key + ")]";
			value = processConditionalSegment(segment, replacements, key);
		}
		else {
			value = "[CONDITIONAL WITH NO CLOSE:" + segment + "(" + segment.substr(segment.size() - 3, 3) + ")]";
		}
	}
	else {
		value = segment;
	}
	return value;
}

// This only looks for one key.
// TODO: Add logic to find multiple keys (returning vector instead of string)
std::string StringTemplate::getConditionalKey(const std::string& segment) {
	size_t colonPosition = segment.find(":", 3);
	std::string key;
	if (colonPosition != std::string::npos) {
		key = segment.substr(3, colonPosition - 3);
	}
	return key;
}

std::string StringTemplate::processConditionalSegment(
	const std::string& segment, const std::map<std::string, std::string>& replacements, const std::string& key) {

	// Ensure the key exists in the replacement map
	if (key.rfind("_999") != (key.size() - 4)) {
		try {
			// This will throw an exception if key does not exist in the map
			replacements.at(key);
		}
		catch (const std::out_of_range&) {
			// don't include segment if key does not exist
			return "";
		}
	}

	// remove conditional portion: 
	//  BEFORE: -={KEY:text-=<KEY2>=-}=-
	//  AFTER.: text-=<KEY2>=-
	size_t colonPos = segment.find(":", 0);
	std::string segmentInner = segment.substr(colonPos + 1, segment.size() - 4 - colonPos);

	// Repeating conditional segment
	std::string value;
	if (key.rfind("_999") == (key.size() - 4)) {
		int iteration = 1;
		while (iteration < 1000) {
			std::string iteration_key = key.substr(0, key.size() - 3) + std::to_string(iteration);
			try {
				// This will throw an exception if iteration_key does not exist in the map
				// ie: if we don't find a key for the current iteration, we are done
				replacements.at(iteration_key);

				// replace -=<KEY_999>=- with iteration key
				// ex: becomes -=<KEY_1>=- within segment if iteration 1
				std::string segmentCopy = replaceAllOccurrences(segmentInner, "_999", "_" + std::to_string(iteration));
				
				// Because segmentCopy does not have -={ }=- around it, it is treated as a normal string (full recursion!)
				value += applyReplacements(segmentCopy, replacements);
			}
			catch (const std::out_of_range&) {
				// iteration_key not found: done
				break;
			}
			iteration++;
		}
	}

	// Non-Repeating conditional segment
	else {
		value += applyReplacements(segmentInner, replacements);
	}

	return value;
}

std::string StringTemplate::replaceAllOccurrences(
	const std::string& segment, const std::string& oldSubstring, const std::string& newSubstring) {
	
	std::string segmentCopy = segment;
	size_t pos = 0;
	while ((pos = segmentCopy.find(oldSubstring, pos)) != std::string::npos) {
		segmentCopy.replace(pos, oldSubstring.length(), newSubstring);
		pos += newSubstring.length();
	}
	return segmentCopy;

}