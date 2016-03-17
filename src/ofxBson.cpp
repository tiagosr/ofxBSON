#include "ofxBson.h"

using namespace _bson;
void ofxBson::serialize(const ofAbstractParameter & parameter) {
	if (!parameter.isSerializable()) {
		return;
	}
	string name = parameter.getEscapedName();
	if (parameter.type() == typeid(ofParameterGroup).name()) {
		const ofParameterGroup & group = static_cast<const ofParameterGroup &>(parameter);
		addChild(name);
		setTo(name);
		for (auto&p : group) {
			serialize(*p);
		}
		setToParent();
	} else if (parameter.type() == typeid(ofParameter<float>).name()) {
		const ofParameter<float> &f = static_cast<const ofParameter<float> &>(parameter);
		setValue(name, f.get());
	}
	else if (parameter.type() == typeid(ofParameter<double>).name()) {
		const ofParameter<double> &d = static_cast<const ofParameter<double> &>(parameter);
		setValue(name, d.get());
	} else if (parameter.type() == typeid(ofParameter<bool>).name()) {
		const ofParameter<bool> &b = static_cast<const ofParameter<bool> &>(parameter);
		setValue(name, b.get());
	} else if (parameter.type() == typeid(ofParameter<string>).name()) {
		setValue(name, parameter.toString());
	}
}

void ofxBson::deserialize(ofAbstractParameter & parameter) {
	if (!parameter.isSerializable()) {
		return;
	}
	string name = parameter.getEscapedName();
	if (parameter.type() == typeid(ofParameterGroup).name()) {
		ofParameterGroup &group = static_cast<ofParameterGroup &>(parameter);
		if (setTo(name)) {
			for (auto& p : group) {
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
	bsonobj obj = root->obj();
	ofBuffer buf = ofBuffer(obj.objdata(), obj.objsize());
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
	auto p = current->getChild(name);
	if (p) {
		current = shared_ptr<BSONObjNode>(p);
		return true;
	}
	return false;
}

void ofxBson::setToParent() {
	auto p = current->getParent();
	if (!p.expired()) {
		current = shared_ptr<BSONObjNode>(p);
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

inline ofxBson::BSONObjNode::BSONObjNode(const _bson::bsonobj & obj) {
	list<bsonelement> elems;
	obj.elems(elems);
	for (auto& elem : elems) {
		string name = elem.fieldName();
		if (elem.isNull()) {
			addNull(name);
		}
		else if (elem.isBoolean()) {
			booleans[name] = elem.boolean();
		}
		else if (elem.isNumber()) {
			doubles[name] = elem.number();
		}
		else if (elem.isObject()) {
			objects[name] = make_shared<BSONObjNode>(elem.object());
		}
		else {
			strings[name] = elem.str();
		}
	}
}

inline bool ofxBson::BSONObjNode::exists(const string & name) const {
	return (nulls.find(name) != nulls.cend()) ||
		(booleans.find(name) != booleans.cend()) ||
		(strings.find(name) != strings.cend()) ||
		(doubles.find(name) != doubles.cend()) ||
		(objects.find(name) != objects.cend());
}

inline bsonobj ofxBson::BSONObjNode::obj() const {
	bsonobjbuilder b;
	for (auto&n : nulls) {
		b.appendNull(n.first);
	}
	for (auto&bl : booleans) {
		b.appendBool(bl.first, bl.second);
	}
	for (auto&d : doubles) {
		b.appendNumber(d.first, d.second);
	}
	for (auto&s : strings) {
		b.append(s.first, s.second);
	}
	for (auto&o : objects) {
		b.append(o.first, o.second->obj());
	}
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
