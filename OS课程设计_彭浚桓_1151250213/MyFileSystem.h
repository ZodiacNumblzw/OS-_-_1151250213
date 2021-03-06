﻿#pragma once

#include <string>
#include <time.h>

using namespace std;


//一些常量的定义
const int BLOCK_SIZE = 64; //磁盘块的大小——64字节

struct Disk;
struct RootDirectory;
struct I_NodeBitmap;
struct BlockBitmap;
struct I_NODE;
union DataBlock;

struct DirectoryEntry;
struct IndexBlock;
struct TxtBlock;
struct DirectoryFileBlock;

//目录项
struct DirectoryEntry {
	char fileName[11];//文件或目录名
	unsigned char flag;//0 表示一般文件，1 表示目录
	int i_node_number;//i-node 编号——指向文件或者目录的索引节点
	DirectoryEntry() {
		fileName[0] = '\0';
		flag = -1;
		i_node_number = -1;
	}

	bool init(char fileName_[11], int flag_, int i_node_number_);
};

//索引数据块
struct IndexBlock {
	int indexs[16];//每个盘块号64字节，可以存放16个索引，即16个blockbitmap的下标
	IndexBlock() {
		for (size_t i = 0; i < BLOCK_SIZE / 4; i++) {
			indexs[i] = -1;
		}
	}
};

//文本文件数据块
struct TxtBlock {
	char txt[BLOCK_SIZE];

	TxtBlock() {
		for (size_t i = 0; i < BLOCK_SIZE; i++) {
			txt[i] = -1;
		}
	}
};

//目录文件数据块
struct DirectoryFileBlock {
	DirectoryEntry direcoryEntry[4];
};

union DataBlock {
	IndexBlock indexBlock;//索引块
	TxtBlock txtBlock;//文本文件数据块
	DirectoryFileBlock directoryBlock;//目录文件数据块
	DataBlock() {

	}
};


//根目录——最多四个目录项
struct RootDirectory {
	DirectoryEntry direcoryEntries[4];
	
	/**
	 * \brief 从根目录中获取一个可用的空的目录项
	 * \param j 整形引用，用于存放找到的目录项下标
	 * \return 如果找到则返回true，反之false
	 */
	bool getAnVoidDirecoryEntry(int& j);
};


//i-node位图
struct I_NodeBitmap {
	bool i_node_bitmap[512]{false};

	/**
	 * \brief 获取一个能够使用的i-node下标
	 * \param i 用于存储下标的整形
	 * \return 如果存在->true
	 */
	bool getAnINodeNum(int& i);
};


//数据块位图——描述磁盘的状态
struct BlockBitmap {
	bool blocks[512 * 2]{false};


	/**
	 * \brief 获取一个能够使用的数据块的下标
	 * \param i 
	 * \return 如果存在->true
	 */

	bool getABlockNum(int& i);
};

//i-node 长度为16字节（1+1+1+1+2*4+4）
//在文件超出两个盘块时，将使用一级索引地址，就是说文件最大为18个盘块
struct I_NODE {
	unsigned char isReadOnly;// 是否只读
	unsigned char isHide;// 是否隐藏
	unsigned char hour;// 建立文件系统时间的小时
	unsigned char minutes;// 建立文件系统时间的分钟
	int directAddress[2];// 直接盘块地址——指向一个数据块
	int firstClassIndexAddress;// 一级索引地址——指向一个索引块

	I_NODE() {
		isReadOnly = '0';
		isHide = '0';

		time_t t = time(nullptr);
		char temp[1];
		strftime(temp, sizeof(temp), "%H", localtime(&t));
		hour = *temp;
		strftime(temp, sizeof(temp), "%M", localtime(&t));
		minutes = *temp;
		directAddress[0] = -1;
		directAddress[1] = -1;
		firstClassIndexAddress = -1;
	}

	void init();


	//判断当前i-node能否继续添加子i-node(子目录或者子文件）
	bool isFull(DataBlock dataBlocks[]);
	//为当前inode所对应的目录增添子目录或文件
	bool addChild(int childINodeNum, DataBlock dataBlocks[], BlockBitmap& blockBitMap, string path, bool isDir);
	//判断指定的子目录是否存在于当前inode中
	bool existChild(string child, DataBlock dataBlocks[], int& inodeNum, DirectoryEntry** direcoryEntry_ = new DirectoryEntry *);
	//清空作为文本文件的当前inode的数据（文本）
	bool clear(DataBlock datablocks[], BlockBitmap& blockBitMap);
	//显示作为文本文件的当前inode的数据（文本）
	void show(DataBlock datablocks[]);
	//增加作为文本文件的当前inode的数据（文本）
	void addText(const string& cs, DataBlock datablok[], BlockBitmap& bitmap);
	//获取作为文本文件的当前inode的数据（文本）
	string getTxt(DataBlock datablocks[]);
};

//磁盘
struct Disk {
	RootDirectory rootDirectory;//根目录
	I_NodeBitmap i_nodeBitMap;//描述512个i-node的状态
	BlockBitmap blockBitMap;//描述1024块磁盘块的状态
	I_NODE i_node_s[512];//512个i-node数组
	DataBlock dataBlocks[1024];//1024个磁盘块数组
};


class InodeLink {
public:
	int val;
	InodeLink* next;

	InodeLink(int i) : val(i) {
		next = nullptr;
	}
};

class InodeLinkManager {
public:
	InodeLink* head;
	InodeLink* tail;

	InodeLinkManager(InodeLink* head_) : head(head_), tail(head_) {

	}

	bool getParent(int& cwd_inode);//获取到当前结点的父节点
	int addInode(int inode_number);


};

class Cmd {
public:

	Cmd(Disk* disk);
	~Cmd();
	bool parse(string cmd);
	static Cmd* getInstance(Disk* disk);
	static string* split(string s, char c);

	Disk* disk;

private:
	int cwd_inode_num;
	string cwd;
	InodeLink* head;
	InodeLinkManager* iNodeManager;
public:
	int getCwdINode() const {
		return cwd_inode_num;
	}

	void setCwdINode(int cwd_inode) {
		this->cwd_inode_num = cwd_inode;
	}

	string getCwd() const {
		return cwd;
	}

	void setCwd(const string& cwd) {
		this->cwd = cwd;
	}

private:

	static Cmd* instance;
	bool Format();//初始化磁盘，划定结构
	bool Mk(int inode,string dir, bool isDir);//创建目录(true)或文件（false）
	bool Cd(string dir);//改变当前目录
	bool DelFile(string filepath,bool del);//删除文件（注意只读属性）
	bool DelDir(string path, int& level);//删除目录
	bool Dir();//列文件目录
	bool Copy(string orign_path, string goal_path);//复制文件到某一路径
	bool Open(string filepath);//打开并编辑文件
	bool Attrib(string file_path, string operation);//更改文件属性（是否为只读、是否被隐藏）
	bool ViewINodeMap() const;//显示当前i-node位示图状况
	bool ViewBlockMap();//显示当前block位示图状况
};


inline Cmd::Cmd(Disk* disc): cwd_inode_num(-1) {
	this->disk = disc;
	head = new InodeLink(-1);
	iNodeManager = new InodeLinkManager(head);
}

inline Cmd::~Cmd() {
}

inline Cmd* Cmd::getInstance(Disk* disc) {
	if (instance == nullptr) {
		instance = new Cmd(disc);

	}

	return instance;
}

//完成命令的分割——默认以空格作为分隔符
inline string* Cmd::split(string s, char c = ' ') {
	string* strings = new string[3];
	string temps(s);
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	for (auto i = 0, j = 0;; j++) {
		if ((i = temps.find_first_of(c)) != string::npos) {
			strings[j] = string(temps, 0, i);
			temps = string(temps, i + 1);
		}
		else {
			strings[j] = string(temps, 0, i);
			break;
		}
	}
	return strings;
}
