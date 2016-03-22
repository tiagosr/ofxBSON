#pragma once

#include "ofMain.h"

#include "bson/bsonobjbuilder.h"

class ofxBson: public ofBaseFileSerializer {
protected:
	class BSONArrayNode;
	class BSONObjNode;
	class BSONNode {
	public:
		weak_ptr<BSONNode> parent;
		BSONNode(weak_ptr<BSONNode> parent = weak_ptr<BSONNode>()) : parent(parent) {}
		virtual ~BSONNode() {}
		virtual bool isObject() const { return false; }
		virtual bool isArray() const { return false; }
		virtual bool isString() const { return false; }
		virtual bool isNumber() const { return false; }
		virtual bool isBool() const { return false; }
		virtual bool isNull() const { return false; }

		virtual string getString() const { return ""; }
		virtual double getNumber() const { return numeric_limits<double>::signaling_NaN(); }
		virtual bool getBool() const { return false; }
		virtual shared_ptr<BSONArrayNode> getArray() { return shared_ptr<BSONArrayNode>(); }
		virtual shared_ptr<BSONObjNode> getObject() { return shared_ptr<BSONObjNode>(); }
	};

	class BSONNullNode : public BSONNode {
	public:
		BSONNullNode(weak_ptr<BSONNode> parent = weak_ptr<BSONNode>()): BSONNode(parent) {}
		bool isNull() const { return true; }
	};

	class BSONStringNode : public BSONNode, public enable_shared_from_this<BSONStringNode> {
	protected:
		string str;
	public:
		BSONStringNode(const string& str = "", weak_ptr<BSONNode> parent = weak_ptr<BSONNode>()) :
			BSONNode(parent), str(str) {}
		bool isString() const { return true; }
		virtual string getString() const { return str; }
	};

	class BSONNumberNode : public BSONNode {
	protected:
		double n;
	public:
		BSONNumberNode(double d = 0.0, weak_ptr<BSONNode> parent = weak_ptr<BSONNode>()):
			BSONNode(parent), n(d) {}
		bool isNumber() const { return true; }
		double getNumber() const { return n; }
	};

	class BSONBoolNode : public BSONNode {
	protected:
		bool b;
	public:
		BSONBoolNode(bool b = false, weak_ptr<BSONNode> parent = weak_ptr<BSONNode>()):
			BSONNode(parent), b(b) {}
		bool isBool() const { return true; }
		bool getBool() const { return b; }
	};
	class BSONArrayNode : public BSONNode, public enable_shared_from_this<BSONArrayNode> {
	protected:
		vector<shared_ptr<BSONNode>> items;
	public:
		BSONArrayNode(weak_ptr<BSONNode> parent = weak_ptr<BSONNode>()):BSONNode(parent) {}
		bool isArray() const { return true; }
		shared_ptr<BSONArrayNode> getArray() { return shared_from_this(); }
		void constructInBuilder(_bson::bsonobjbuilder &builder) {
			int c = 0;
			for (auto& item : items) {
				if (item->isNull()) {
					builder.appendNull(ofToString(c));
				} else if (item->isBool()) {
					builder.appendBool(ofToString(c), item->getBool());
				} else if (item->isNumber()) {
					builder.appendNumber(ofToString(c), item->getNumber());
				} else if (item->isString()) {
					builder.append(ofToString(c), item->getString());
				} else if (item->isArray()) {
					_bson::bsonobjbuilder b(builder.subarrayStart(ofToString(c)));
					item->getArray()->constructInBuilder(b);
					b._done();
				}
				c++;
			}
		}
	};
	class BSONObjNode: public BSONNode, public enable_shared_from_this<BSONObjNode> {
	protected:
		map<string, shared_ptr<BSONNode>> content;
	public:
		BSONObjNode() {}
		BSONObjNode(weak_ptr<BSONNode> parent): BSONNode(parent) {}
		BSONObjNode(const _bson::bsonobj& obj, weak_ptr<BSONNode> _parent = weak_ptr<BSONNode>());
		void addChild(const string& name) {
			content[name] = make_shared<BSONObjNode>(this);
		}
		void addNull(const string& name) {
			content[name] = make_shared<BSONNullNode>(this);
		}
		void addString(const string& name, const string& value) {
			content[name] = make_shared<BSONStringNode>(value, this);
		}
		void addNumber(const string& name, double value) {
			content[name] = make_shared<BSONNumberNode>(value, this);
		}
		void addBool(const string& name, bool value) {
			content[name] = make_shared<BSONBoolNode>(value, this);
		}

		bool exists(const string& name) const;
		shared_ptr<BSONNode> getChild(const string& name) const {
			return content.find(name)->second;
		}
		weak_ptr<BSONNode> getParent() const {
			return parent;
		}
		shared_ptr<BSONObjNode> getObject() {
			return shared_from_this();
		}
		double getNumber(const string& name) const {
			return getChild(name)->getNumber();
		}
		bool getBool(const string& name) const {
			return getChild(name)->getBool();
		}
		string getString(const string& name) const {
			return getChild(name)->getString();
		}
		shared_ptr<BSONArrayNode> getArray(const string& name) const {
			return getChild(name)->getArray();
		}

		bool isChild(const string& name) const {
			return (content.find(name) != content.cend());
		}
		bool isNull(const string& name) const {
			return isChild(name) && getChild(name)->isNull();
		}
		bool isBool(const string& name) const {
			return isChild(name) && getChild(name)->isBool();
		}
		bool isNumber(const string& name) const {
			return isChild(name) && getChild(name)->isNumber();
		}
		bool isString(const string& name) const {
			return isChild(name) && getChild(name)->isString();;
		}
		bool isObject(const string& name) const {
			return isChild(name) && getChild(name)->isObject();
		}
		bool isArray(const string& name) const {
			return isChild(name) && getChild(name)->isArray();
		}
		void constructInBuilder(_bson::bsonobjbuilder& b) const;
		_bson::bsonobj obj() const;
	};
	shared_ptr<BSONObjNode> root;
	shared_ptr<BSONObjNode> current;
public:
	ofxBson(): root(make_shared<BSONObjNode>()), current(root) {}
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