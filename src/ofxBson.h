#pragma once

#include "ofMain.h"

#include "bson/bsonobjbuilder.h"

class ofxBson: public ofBaseFileSerializer {
protected:
	class BSONObjNode: public enable_shared_from_this<BSONObjNode> {
	protected:
		map<string, shared_ptr<BSONObjNode>> objects;
		map<string, string> strings;
		map<string, double> doubles;
		map<string, bool> booleans;
		map<string, bool> nulls;
		weak_ptr<BSONObjNode> parent;
	public:
		BSONObjNode() {}
		BSONObjNode(weak_ptr<BSONObjNode> parent): parent(parent) {}
		BSONObjNode(const _bson::bsonobj& obj);
		BSONObjNode(const _bson::bsonobj& obj, weak_ptr<BSONObjNode> _parent): BSONObjNode(obj) {
			parent = _parent;
		}
		void addChild(const string& name) {
			objects[name] = make_shared<BSONObjNode>();
		}
		void addNull(const string& name) {
			nulls[name] = true;
		}
		void addString(const string& name, const string& value) {
			strings[name] = value;
		}
		void addNumber(const string& name, double value) {
			doubles[name] = value;
		}
		void addBool(const string& name, bool value) {
			booleans[name] = value;
		}

		bool exists(const string& name) const;
		shared_ptr<BSONObjNode> getChild(const string& name) const {
			return objects.find(name)->second;
		}
		weak_ptr<BSONObjNode> getParent() const {
			return parent;
		}

		double getNumber(const string& name) const {
			return doubles.find(name)->second;
		}
		bool getBool(const string& name) const {
			return booleans.find(name)->second;
		}
		string getString(const string& name) const {
			return strings.find(name)->second;
		}

		bool isNull(const string& name) const {
			return (nulls.find(name) != nulls.cend());
		}
		bool isBool(const string& name) const {
			return (booleans.find(name) != booleans.cend());
		}
		bool isNumber(const string& name) const {
			return (doubles.find(name) != doubles.cend());
		}
		bool isString(const string& name) const {
			return (strings.find(name) != strings.cend());
		}
		bool isObject(const string& name) const {
			return (objects.find(name) != objects.cend());
		}

		_bson::bsonobj obj() const;
	};
	shared_ptr<BSONObjNode> root;
	shared_ptr<BSONObjNode> current;
public:
	bool exists(const string& name) const;
	void addChild(const string& name);
	bool setTo(const string& name);
	void setToParent();
	void setValue(const string& name, const string& value);
	void setValue(const string& name, double value);
	void setValue(const string& name, bool value);
	void setNull(const string& name);

	int getIntValue(const string& name) const;
	float getFloatValue(const string& name) const;
	double getDoubleValue(const string& name) const;
	int64_t getInt64Value(const string& name) const;
	bool getBoolValue(const string& name) const;
	string getValue(const string& name) const;

	virtual void serialize(const ofAbstractParameter & parameter) override;
	virtual void deserialize(ofAbstractParameter & parameter) override;
	virtual bool load(const string & path) override;
	virtual bool save(const string & path) override;
};