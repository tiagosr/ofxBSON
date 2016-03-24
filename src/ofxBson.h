#pragma once

#include "ofMain.h"

#include "bson/bsonobjbuilder.h"

class ofxBson: public ofBaseFileSerializer {
protected:
	class BSONArrayNode;
	class BSONObjNode;
	class BSONNode {
	public:
		static ofBuffer emptyBuffer;
		weak_ptr<BSONNode> parent;
		BSONNode(weak_ptr<BSONNode> parent = weak_ptr<BSONNode>()) : parent(parent) {}
		virtual ~BSONNode() {}
		virtual bool isObject() const { return false; }
		virtual bool isArray() const { return false; }
		virtual bool isString() const { return false; }
		virtual bool isNumber() const { return false; }
		virtual bool isInt32() const { return false; }
		virtual bool isInt64() const { return false; }
		virtual bool isBuffer() const { return false; }
		virtual bool isBool() const { return false; }
		virtual bool isNull() const { return false; }
		virtual bool isUndefined() const { return false; }

		virtual string getString() const { return ""; }
		virtual double getNumber() const { return numeric_limits<double>::signaling_NaN(); }
		virtual bool getBool() const { return false; }
		virtual int32_t getInt32() const { return numeric_limits<int32_t>::min(); }
		virtual int64_t getInt64() const { return numeric_limits<int64_t>::min(); }
		virtual const ofBuffer& getBuffer() const { return emptyBuffer; }
		virtual shared_ptr<BSONArrayNode> getArray() { return shared_ptr<BSONArrayNode>(); }
		virtual shared_ptr<BSONObjNode> getObject() { return shared_ptr<BSONObjNode>(); }
		weak_ptr<BSONNode> getParent() const {
			return parent;
		}
	};

	class BSONUndefinedNode : public BSONNode {
	public:
		BSONUndefinedNode(weak_ptr<BSONNode> parent = weak_ptr<BSONNode>()) : BSONNode(parent) {}
		bool isUndefined() const { return true; }
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
	class BSONInt32Node : public BSONNode {
	protected:
		int32_t n;
	public:
		BSONInt32Node(int32_t n = 0, weak_ptr<BSONNode> parent = weak_ptr<BSONNode>()) :
			BSONNode(parent), n(n) {
		}
		bool isInt32() const { return true; }
		int32_t getInt32() const { return n; }
		int64_t getInt64() const { return n; }
	};
	class BSONInt64Node : public BSONNode {
	protected:
		int64_t n;
	public:
		BSONInt64Node(int64_t n = 0, weak_ptr<BSONNode> parent = weak_ptr<BSONNode>()) :
			BSONNode(parent), n(n) {
		}
		bool isInt64() const { return true; }
		int64_t getInt64() const { return n; }
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

	class BSONBufferNode : public BSONNode {
	protected:
		ofBuffer buf;
	public:
		BSONBufferNode(const ofBuffer& b, weak_ptr<BSONNode> parent = weak_ptr<BSONNode>()):
			BSONNode(parent), buf(b) { }
		bool isBuffer() const { return true; }
		const ofBuffer& getBuffer() const { return buf; }
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
				} else if (item->isInt32()) {
					builder.appendIntOrLL(ofToString(c), item->getInt32());
				} else if (item->isInt64()) {
					builder.append(ofToString(c), item->getInt64());
				} else if (item->isBuffer()) {
					builder.appendBinData(ofToString(c), item->getBuffer().size(), _bson::BinDataType::BinDataGeneral, item->getBuffer().getBinaryBuffer());
				} else if (item->isString()) {
					builder.append(ofToString(c), item->getString());
				} else if (item->isArray()) {
					_bson::bsonobjbuilder b(builder.subarrayStart(ofToString(c)));
					item->getArray()->constructInBuilder(b);
					b._done();
				} else if (item->isObject()) {
					_bson::bsonobjbuilder b(builder.subobjStart(ofToString(c)));
					item->getObject()->constructInBuilder(b);
					b._done();
				}
				c++;
			}
		}
		size_t length() const { return items.size(); }
		shared_ptr<BSONNode> getAt(size_t i) const {
			if (i < length()) {
				return items[i];
			} else {
				return shared_ptr<BSONNode>();
			}
		}
		size_t push(shared_ptr<BSONNode> node) {
			items.push_back(node);
			return items.size() - 1;
		}

		size_t pushNull() {
			return push(make_shared<BSONNullNode>(shared_from_this()));
		}
		size_t pushBool(bool b) {
			return push(make_shared<BSONBoolNode>(b, shared_from_this()));
		}
		size_t pushNumber(double d) {
			return push(make_shared<BSONNumberNode>(d, shared_from_this()));
		}
		size_t pushInt32(int32_t i) {
			return push(make_shared<BSONInt32Node>(i, shared_from_this()));
		}
		size_t pushInt64(int64_t i) {
			return push(make_shared<BSONInt64Node>(i, shared_from_this()));
		}
		size_t pushString(const string& str) {
			return push(make_shared<BSONStringNode>(str, shared_from_this()));
		}
		size_t pushNewObject() {
			return push(make_shared<BSONObjNode>(shared_from_this()));
		}
		size_t pushNewArray() {
			return push(make_shared<BSONArrayNode>(shared_from_this()));
		}
		size_t pushBuffer(const ofBuffer& buf) {
			return push(make_shared<BSONBufferNode>(buf, shared_from_this()));
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
			content[name] = make_shared<BSONObjNode>(shared_from_this());
		}
		void addNull(const string& name) {
			content[name] = make_shared<BSONNullNode>(shared_from_this());
		}
		void addString(const string& name, const string& value) {
			content[name] = make_shared<BSONStringNode>(value, shared_from_this());
		}
		void addNumber(const string& name, double value) {
			content[name] = make_shared<BSONNumberNode>(value, shared_from_this());
		}
		void addInt32(const string& name, int32_t value) {
			content[name] = make_shared<BSONInt32Node>(value, shared_from_this());
		}
		void addInt64(const string& name, int64_t value) {
			content[name] = make_shared<BSONInt64Node>(value, shared_from_this());
		}
		void addBool(const string& name, bool value) {
			content[name] = make_shared<BSONBoolNode>(value, shared_from_this());
		}
		void addBuffer(const string& name, const ofBuffer& buf) {
			content[name] = make_shared<BSONBufferNode>(buf, shared_from_this());
		}
		void addArray(const string& name) {
			content[name] = make_shared<BSONArrayNode>(shared_from_this());
		}
		bool exists(const string& name) const;
		shared_ptr<BSONNode> getChild(const string& name) const {
			return content.find(name)->second;
		}
		shared_ptr<BSONObjNode> getObject() {
			return shared_from_this();
		}
		double getNumber(const string& name) const {
			return getChild(name)->getNumber();
		}
		int32_t getInt32(const string& name) const {
			return getChild(name)->getInt32();
		}
		int64_t getInt64(const string& name) const {
			return getChild(name)->getInt64();
		}
		const ofBuffer& getBuffer(const string& name) const {
			return getChild(name)->getBuffer();
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
		bool isInt32(const string& name) const {
			return isChild(name) && getChild(name)->isInt32();
		}
		bool isInt64(const string& name) const {
			return isChild(name) && getChild(name)->isInt64();
		}
		bool isBuffer(const string& name) const {
			return isChild(name) && getChild(name)->isBuffer();
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
	shared_ptr<BSONNode> root;
	shared_ptr<BSONNode> current;

public:
	ofxBson(): root(make_shared<BSONObjNode>()), current(root) {}

	bool exists(const string& name) const;
	bool exists(size_t index) const;
	void addChild(const string& name);
	size_t addChildToArray();
	void addArray(const string& name);
	size_t addArrayToArray();
	bool setTo(const string& name);
	bool setTo(size_t index);
	void setToParent();
	void setValue(const string& name, const string& value);
	size_t pushValue(const string& value);
	void setValue(const string& name, double value);
	size_t pushValue(double value);
	void setValue(const string& name, bool value);
	size_t pushValue(bool value);
	void setValue(const string& name, int32_t value);
	size_t pushValue(int32_t value);
	void setValue(const string& name, int64_t value);
	size_t pushValue(int64_t value);
	void setNull(const string& name);
	size_t pushNull();
	size_t pushObject();
	size_t pushArray();
	void setBuffer(const string& name, const ofBuffer& value);
	size_t pushBuffer(const ofBuffer& buf);

	size_t getSize() const;

	int getIntValue(const string& name) const;
	int getIntValue(size_t index) const;
	float getFloatValue(const string& name) const;
	float getFloatValue(size_t index) const;
	double getDoubleValue(const string& name) const;
	double getDoubleValue(size_t index) const;
	int64_t getInt64Value(const string& name) const;
	int64_t getInt64Value(size_t index) const;
	bool getBoolValue(const string& name) const;
	bool getBoolValue(size_t index) const;
	string getValue(const string& name) const;
	string getValue(size_t index) const;

	virtual void serialize(const ofAbstractParameter & parameter) override;
	virtual void deserialize(ofAbstractParameter & parameter) override;
	virtual bool load(const string & path) override;
	virtual bool save(const string & path) override;
};