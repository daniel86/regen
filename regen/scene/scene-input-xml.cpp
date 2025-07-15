/*
 * scene-input-xml.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "scene-input-xml.h"
#include "resource-processor.h"
#include <regex>

using namespace regen::scene;
using namespace regen;
using namespace std;

// Function to replace placeholders with actual values
string preprocessXML(const string &xmlContent, const map<string, string> &variables) {
    stringstream input(xmlContent);
    stringstream output;
    string line;

    while (getline(input, line)) {
        for (const auto &var : variables) {
            string placeholder = "{{" + var.first + "}}";
            if (line.find(placeholder) != string::npos) {
                string escapedPlaceholder = regex_replace(placeholder, regex(R"([-[\]{}()*+?.,\^$|#\s])"), R"(\$&)");
                line = regex_replace(line, regex(escapedPlaceholder), var.second);
            }
        }
        output << line << '\n';
    }

    return output.str();
}

// Function to extract global constants from the XML content
map<string, string> extractGlobalConstants(const string &xmlContent) {
    map<string, string> variables;
    regex constantRegex(R"lit(<constant\s+name="([^"]+)"\s+value="([^"]+)"\s*/>)lit");
    smatch match;
    string::const_iterator searchStart(xmlContent.cbegin());

    while (regex_search(searchStart, xmlContent.cend(), match, constantRegex)) {
        variables[match[1].str()] = match[2].str();
        searchStart = match.suffix().first;
    }

    return variables;
}

SceneInputXML::SceneInputXML(const string &xmlFile) {
	inputFile_ = xmlFile;
	xmlInput_.open(inputFile_.c_str(), ios::in);
	buffer_ = vector<char>((
								   istreambuf_iterator<char>(xmlInput_)),
						   istreambuf_iterator<char>());
	buffer_.push_back('\0');

	// Preprocess the XML content
    string xmlContent(buffer_.begin(), buffer_.end());
    auto variables = extractGlobalConstants(xmlContent);
    string preprocessedXML = preprocessXML(xmlContent, variables);
    // Update buffer with preprocessed XML content
    buffer_ = vector<char>(preprocessedXML.begin(), preprocessedXML.end());
    buffer_.push_back('\0');

	try {
		doc_.parse<0>(&buffer_[0]);
		SceneInputNodeXML *nullParent = nullptr;
		rootNode_ = ref_ptr<SceneInputNodeXML>::alloc(nullParent, &doc_);
	}
	catch (rapidxml::parse_error &exc) {
		REGEN_ERROR("Failed to parse XML document.");
		REGEN_ERROR(exc.what());
		rootNode_ = ref_ptr<SceneInputNodeXML>::alloc();
	}
}

ref_ptr<SceneInputNode> SceneInputXML::getRoot() { return rootNode_; }

////////////////////
////////////////////

SceneInputNodeXML::SceneInputNodeXML()
		: SceneInputNode(nullptr),
		  xmlNode_(nullptr) {
	category_ = "";
	name_ = "";
}

static inline std::string getNodeID(rapidxml::xml_node<> *node) {
	rapidxml::xml_attribute<> *a = node->first_attribute("id");
	if (a != nullptr) {
		return a->value();
	} else {
		// convert pointer to string
		return REGEN_STRING(reinterpret_cast<std::uintptr_t>(node));
	}
}

SceneInputNodeXML::SceneInputNodeXML(
		SceneInputNodeXML *parent,
		rapidxml::xml_node<> *xmlNode)
		: SceneInputNode(parent),
		  xmlNode_(xmlNode) {
	category_ = xmlNode_->name();
	name_ = getNodeID(xmlNode);
}

string SceneInputNodeXML::getCategory() const { return category_; }

string SceneInputNodeXML::getName() const { return name_; }

void SceneInputNodeXML::removeChild(const ref_ptr<SceneInputNode> &child) {
	// first remove from the children list
	for (auto it = children_.begin(); it != children_.end(); ++it) {
		if (it->get() == child.get()) {
			children_.erase(it);
			break;
		}
	}

	auto xmlChild = dynamic_cast<SceneInputNodeXML *>(child.get());
	if (xmlChild != nullptr) {
		// next remove from the XML node
		xmlNode_->remove_node(xmlChild->xmlNode_);
	}
}

const list<ref_ptr<SceneInputNode> > &SceneInputNodeXML::getChildren() {
	if (xmlNode_ != nullptr && children_.empty()) {
		for (rapidxml::xml_node<> *n = xmlNode_->first_node(); n != nullptr; n = n->next_sibling()) {
			string category(n->name());
			// Handle include nodes.
			if (category == "include") {
				rapidxml::xml_attribute<> *a = n->first_attribute("xml-file");
				if (a != nullptr) {
					string fileName(a->value());
					string filePath = resourcePath(fileName);

					ref_ptr<SceneInputXML> included =
							ref_ptr<SceneInputXML>::alloc(filePath);
					inclusions_.push_back(included);

					auto &x = included->getRoot()->getChildren();
					if (x.size() == 1) {
						auto &y = x.front()->getChildren();
						children_.insert(children_.end(), y.begin(), y.end());
					} else if (x.size() > 1) {
						children_.insert(children_.end(), x.begin(), x.end());
					} else {
						REGEN_WARN("No children found in included XML file: " + filePath);
					}
				} else {
					REGEN_WARN("Unable to include XML file. Missing 'xml-file' attribute.");
				}
			}
				// Handle other nodes.
			else {
				children_.emplace_back(ref_ptr<SceneInputNodeXML>::alloc(this, n));
			}
		}
	}
	return children_;
}

const map<string, string> &SceneInputNodeXML::getAttributes() {
	if (xmlNode_ != nullptr && attributes_.empty()) {
		for (rapidxml::xml_attribute<> *a = xmlNode_->first_attribute(); a != nullptr; a = a->next_attribute()) {
			attributes_[a->name()] = a->value();
		}
	}
	return attributes_;
}
