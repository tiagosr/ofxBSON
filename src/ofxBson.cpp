#include "ofxBson.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

using namespace _bson;

ofBuffer ofxBson::BSONNode::emptyBuffer;

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
	root->getObject()->constructInBuilder(b);
	ofBuffer buf = ofBuffer(b.obj().objdata(), b.obj().objsize());
	toSave.writeFromBuffer(buf);
	toSave.close();
	return true;
}


bool ofxBson::exists(const string & name) const {
	return current->getObject()->exists(name);
}

bool ofxBson::exists(size_t index) const {
	return !!current->getArray()->getAt(index);
}

void ofxBson::addChild(const string& name) {
	current->getObject()->addChild(name);
}

size_t ofxBson::addChildToArray() {
	return current->getArray()->push(make_shared<BSONObjNode>(current));
}

void ofxBson::addArray(const string & name) {
	return current->getObject()->addArray(name);
}

size_t ofxBson::addArrayToArray() {
	return current->getArray()->push(make_shared<BSONArrayNode>(current));
}

bool ofxBson::setTo(const string & name) {
	auto o = current->getObject();
	if (!o) return false;
	auto p = o->getChild(name)->getObject();
	if (p) {
		current = p;
		return true;
	}
	return false;
}

bool ofxBson::setTo(size_t index) {
	auto o = current->getArray();
	if (!o) return false;
	auto i = o->getAt(index);
	if (i) {
		current = i;
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
	current->getObject()->addString(name, value);
}
size_t ofxBson::pushValue(const string & value) {
	return current->getArray()->pushString(value);
}
void ofxBson::setValue(const string & name, double value) {
	current->getObject()->addNumber(name, value);
}

size_t ofxBson::pushValue(double value) {
	return current->getArray()->pushNumber(value);
}

void ofxBson::setValue(const string & name, bool value) {
	current->getObject()->addBool(name, value);
}

size_t ofxBson::pushValue(bool value) {
	return current->getArray()->pushBool(value);
}

void ofxBson::setValue(const string & name, int32_t value) {
	return current->getObject()->addInt32(name, value);
}

void ofxBson::setValue(const string & name, int64_t value) {
	return current->getObject()->addInt64(name, value);
}

void ofxBson::setNull(const string & name) {
	return current->getObject()->addNull(name);
}

size_t ofxBson::pushNull() {
	return current->getArray()->pushNull();
}

size_t ofxBson::pushObject() {
	return current->getArray()->pushNewObject();
}

size_t ofxBson::pushArray() {
	return current->getArray()->pushNewArray();
}

void ofxBson::setBuffer(const string & name, const ofBuffer & value) {
	current->getObject()->addBuffer(name, value);
}

void ofxBson::setGUIDObject(const string & name, const string & guid, bool & already_in_store) {
	current->getObject()->addGUIDObject(name, guid, "", already_in_store);
}

void ofxBson::setGUIDObject(const string & name, const string & guid, const string & type, bool & already_in_store) {
	current->getObject()->addGUIDObject(name, guid, type, already_in_store);
}

size_t ofxBson::pushGUIDObject(const string & guid, bool & already_in_store) {
	return current->getArray()->pushGUIDObject(guid, already_in_store);
}

size_t ofxBson::getSize() const {
	return current->getArray()->length();
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
			auto i_obj = elem.object();
			if (i_obj.hasField("%type")) {
				content[name] = make_shared<BSONObjWithGUIDNode>(i_obj, shared_from_this());
			} else {
				content[name] = make_shared<BSONObjNode>(elem.object(), shared_from_this());
			}
		}
		else {
			switch (elem.type()) {
			case BSONType::BinData:
				{
					int len = 0;
					auto chstr = elem.binData(len);
					switch (elem.binDataType()) {
					case BinDataType::newUUID:
						boost::uuids::uuid uid;
						memcpy(uid.data, chstr, 16);
						content[name] = make_shared<BSONGUIDNode>(boost::lexical_cast<string>(uid), shared_from_this());
						break;
					case BinDataType::BinDataGeneral:
					{
						ofBuffer buf(chstr, len);
						content[name] = make_shared<BSONBufferNode>(buf, shared_from_this());
					}
						break;
					default:
						addString(name, elem.String());
					}
				}
			break;
			case BSONType::jstOID:
				content[name] = make_shared<BSONGUIDNode>(elem.__oid().str(), shared_from_this());
				break;
			}
			
		}
	}
}

void ofxBson::BSONObjNode::addGUIDObject(const string & name, const string & guid, const string& type, bool & already_in_store) {
	already_in_store = (bson->storedObjects.find(guid) != bson->storedObjects.cend());
	content[name] = make_shared<BSONGUIDNode>(guid, type, shared_ptr<BSONNode>(this), bson);
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
		} else if (item.second->isGUID()) {
			boost::uuids::uuid uid = boost::lexical_cast<boost::uuids::uuid>(item.second->getGUID());
			
			b.appendBinData(item.first, uid.size(), BinDataType::newUUID, uid.data);
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
	return current->getObject()->getNumber(name);
}

float ofxBson::getFloatValue(const string & name) const {
	return current->getObject()->getNumber(name);
}

double ofxBson::getDoubleValue(const string & name) const {
	return current->getObject()->getNumber(name);
}

bool ofxBson::getBoolValue(const string & name) const {
	return current->getObject()->getBool(name);
}

string ofxBson::getValue(const string & name) const {
	return current->getObject()->getString(name);
}

string ofxBson::getGUID(const string & name) const {
	return current->getObject()->getChild(name)->getGUID();
}

string ofxBson::getGUID(size_t index) const {
	return current->getArray()->getAt(index)->getGUID();
}



size_t ofxBson::BSONArrayNode::pushGUIDObject(const string & guid, bool &already_in_store) {
	already_in_store = (bson->storedObjects.find(guid) != bson->storedObjects.cend());
	return push(make_shared<BSONGUIDNode>(guid, "", shared_ptr<BSONNode>(this), bson));
}

shared_ptr<ofxBson::BSONObjNode> ofxBson::BSONGUIDNode::getObject() const {
	return reference;
}

shared_ptr<void> ofxBson::BSONGUIDNode::getConstructedObject(ofxBson& b) const {
	if (reference) {
		return reference->construct(b);
	} else {
		return shared_ptr<void>();
	}
}

shared_ptr<void> ofxBson::BSONObjWithGUIDNode::construct(ofxBson & b) {
	if (!constructedObject) {
		auto cons = b.constructors.find(type);
		if (cons != b.constructors.cend()) {
			constructedObject = cons->second(b);
			return constructedObject;
		}
		return shared_ptr<void>();
	} else {
		return constructedObject;
	}
}

ofxBson::BSONObjWithGUIDNode::BSONObjWithGUIDNode(const _bson::bsonobj & obj, weak_ptr<BSONNode> parent): BSONObjNode(obj, parent) {
	int len = 0;
	auto guid_c = obj.getField("%guid").binData(len);
	guid = string(guid_c, len);
	type = obj.getStringField("%type");
}

void ofxBson::BSONObjWithGUIDNode::constructInBuilder(_bson::bsonobjbuilder & b) const {
	b.append("%guid", _bson::OID(guid));
	b.append("%type", type);
	ofxBson::BSONObjNode::constructInBuilder(b);
}
