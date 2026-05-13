#include <string>
#include <vector>
#include <iostream>

// Simple tree structure for testing purposes
struct Node {
    std::string value;

    std::vector<Node*> children;
	Node* parent = nullptr;

    Node(const std::string& v = "") : value(v) {}
    ~Node() { 
        for(auto& child : children)
            delete child;
    }

    Node* addChild(const char* v) {
		Node* child = new Node(v);
		child->parent = this;
        children.push_back(child);
		return child;
    }

    operator const char*() const {
        return value.c_str();
	}

    bool operator==(const char* v) const {
        return value == v;
	}

    const std::string& operator[] (size_t index) const {
        if (index < children.size())
            return children[index]->value;
        throw std::out_of_range("Child index out of range");
	}

    void tag() const {
        std::cout << "{ " << path() << " }\n";
	}

    std::string path() const {
        return !parent ? value : parent->path() + " > " + value;
    }

    bool Find(const char* v) const;
};

struct Tree : Node {
    Tree() {
		value = "C";          // C (root)
        Node* c_eb = addChild("Eb"); // C > Eb (minor third)
        Node* c_e =  addChild("E");  // C > E (major third)
        Node* c_f =  addChild("F");  // C > F (perfect fourth)
        c_eb->addChild("G");  // C > Eb > G (C minor)
        c_e->addChild("G")    // C > E > G (C major)
            ->addChild("B");  // C > E > G > B (C major 7th)
        c_e->addChild("A");   // C > E > A (C major)
        c_f->addChild("A");   // C > F > A (F major)
    }
};