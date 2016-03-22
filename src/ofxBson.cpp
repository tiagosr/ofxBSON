#include "ofxBson.h"

using namespace _bson;
void ofxBson::serialize(const ofAbstractParameter & parameter) {
	if (!parameter.isSerializable()) {
		return;
	}
	string name = parameter.getEscapedName();
	if (parameter.type() == typeid(ofParameter<float>).name()) {
		const ofParameter<float> &f = static_cast<const ofParameter<float> &>(parameter);
		setValue(name, f.get());
	} else if (parameter.type() == typeid(ofParameter<double>).name()) {
		const ofParameter<double> &d = static_cast<const ofParameter<double> &>(parameter);
		setValue(name, d.get());
	} else if (parameter.type() == typeid(ofParameter<bool>).name()) {
		const ofParameter<bool> &b = static_cast<const ofParameter<bool> &>(parameter);
		setValue(name, b.get());
	} else if (parameter.type() == typeid(ofParameter<string>).name()) {
		setValue(name, parameter.toString());
	} else {
		const ofParameterGroup *group = dynamic_cast<const ofParameterGroup *>(&parameter);
		if ( group != NULL) {
			addChild(name);
			setTo(name);
			for (auto&p : parameter.castGroup()) {
				serialize(*p);
			}
			setToParent();
		} 
	}
}

void ofxBson::deserialize(ofAbstractParameter & parameter) {
	if (!parameter.isSerializable()) {
		return;
	}
	string name = parameter.getEscapedName();
	const ofParameterGroup *group = dynamic_cast<const ofParameterGroup *>(&parameter);
	if (group != NULL) {
		if (setTo(name)) {
			for (auto& p : parameter.castGroup()) {
				deserialize(*p);
			}
			setToParent();
		}
	}
	else {
		if (exists(name)) {
			if (parameter.type() == typeid(ofParameter<int>).name()) {
				parameter.cast<int>() = getIntValue(name);
			} else if (parameter.type() == typeid(ofParameter<float>).name()) {
				parameter.cast<float>() = getFloatValue(name);
			} else if (parameter.type() == typeid(ofParameter<double>).name()) {
				parameter.cast<double>() = getDoubleValue(name);
			} else if (parameter.type() == typeid(ofParameter<bool>).name()) {
				parameter.cast<bool>() = getBoolValue(name);
			} else {
				parameter.fromString(getValue(name));
			}
		}
	}
}

bool ofxBson::load(const string & path) {
	ofFile loaded;
	if (loaded.open(path, ofFile::ReadOnly, true)) {
		ofBuffer buf = loaded.readToBuffer();
		bsonobj obj(buf.getData());
		current = root = make_shared<BSONObjNode>(obj);
		loaded.close();
		return true;
	}
	return false;
}

bool ofxBson::save(const string & path) {
	ofFile toSave(path, ofFile::Mode::WriteOnly, true);
	bsonobjbuilder b;
	root->constructInBuilder(b);
	ofBuffer buf = ofBuffer(b.obj().objdata(), b.obj().objsize());
	toSave.writeFromBuffer(buf);
	toSave.close();
	return true;
}


bool ofxBson::exists(const string & name) const {
	return current->exists(name);
}

void ofxBson::addChild(const string& name) {
	current->addChild(name);
}

bool ofxBson::setTo(const string & name) {
	auto p = current->getChild(name)->getObject();
	if (p) {
		current = p;
		return true;
	}
	return false;
}

void ofxBson::setToParent() {
	auto p = current->getParent();
	if (!p.expired()) {
		current = p.lock()->getObject();
	}
}
void ofxBson::setValue(const string & name, const string & value) {
	current->addString(name, value);
}
void ofxBson::setValue(const string & name, double value) {
	current->addNumber(name, value);
}

void ofxBson::setValue(const string & name, bool value) {
	current->addBool(name, value);
}

ofxBson::BSONObjNode::BSONObjNode(const _bson::bsonobj & obj,  weak_ptr<BSONNode> _parent):
	BSONNode(_parent)
{
	list<bsonelement> elems;
	obj.elems(elems);
	for (auto& elem : elems) {
		string name = elem.fieldName();
		if (elem.isNull()) {
			addNull(name);
		}
		else if (elem.isBoolean()) {
			addBool(name, elem.boolean());
		}
		else if (elem.isNumber()) {
			addNumber(name,elem.number());
		}
		else if (elem.isObject()) {
			content[name] = make_shared<BSONObjNode>(elem.object());
		}
		else {
			addString(name, elem.String());
		}
	}
}

inline bool ofxBson::BSONObjNode::exists(const string & name) const {
	return (content.find(name) != content.cend());
}

inline void ofxBson::BSONObjNode::constructInBuilder(bsonobjbuilder & b) const {
	for (auto&item : content) {
		if (item.second->isObject()) {
			if (item.second->getObject()) {
				string name = item.first;
				bsonobjbuilder sub(b.subobjStart(name));
				item.second->getObject()->constructInBuilder(sub);
			}
			else b.appendNull(item.first);
		} else if (item.second->isArray()) {
			if (item.second->getArray()) {
				string name = item.first;
				//bsonarraybuilder arr = b.subarrayStart(name);
				bsonobjbuilder arr(b.subarrayStart(name));
				item.second->getArray()->constructInBuilder(arr);
				arr.done();
			}
		} else if (item.second->isNull()) {
			b.appendNull(item.first);
		} else if (item.second->isBool()) {
			b.appendBool(item.first, item.second->getBool());
		} else if (item.second->isNumber()) {
			b.appendNumber(item.first, item.second->getNumber());
		} else if (item.second->isString()) {
			b.append(item.first, item.second->getString());
		}
	}
	b.done();
}

inline bsonobj ofxBson::BSONObjNode::obj() const {
	if (content.size() == 0) {
		return bsonobj();
	}
	bsonobjbuilder b;
	constructInBuilder(b);
	return b.obj();
}

int ofxBson::getIntValue(const string & name) const {
	return current->getNumber(name);
}

float ofxBson::getFloatValue(const string & name) const {
	return current->getNumber(name);
}

double ofxBson::getDoubleValue(const string & name) const {
	return current->getNumber(name);
}

bool ofxBson::getBoolValue(const string & name) const {
	return current->getBool(name);
}

string ofxBson::getValue(const string & name) const {
	return current->getString(name);
}
