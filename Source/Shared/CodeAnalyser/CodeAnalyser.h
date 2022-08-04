#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <vector>


enum class LabelType;

// CPU abstraction
enum class ECPUType
{
	Unknown,
	Z80,
	M6502
};

/* the input callback type */
typedef uint8_t(*FDasmInput)(void* user_data);
/* the output callback type */
typedef void (*FDasmOutput)(char c, void* user_data);

class ICPUInterface
{
public:
	virtual uint8_t		ReadByte(uint16_t address) = 0;
	virtual uint16_t	ReadWord(uint16_t address) = 0;
	virtual uint16_t	GetPC(void) = 0;

	// commands
	virtual void	Break() = 0;
	virtual void	Continue() = 0;
	virtual void	GraphicsViewerSetAddress(uint16_t address) = 0;

	virtual bool	ExecThisFrame(void) = 0;

	virtual void InsertROMLabels(struct FCodeAnalysisState& state) = 0;
	virtual void InsertSystemLabels(struct FCodeAnalysisState& state) = 0;

	ECPUType	CPUType = ECPUType::Unknown;
};

enum class LabelType
{
	Data,
	Function,
	Code,
};

enum class ItemType
{
	Label,
	Code,
	Data,
};

struct FItem
{
	ItemType		Type;
	std::string		Comment;
	uint16_t		Address;
	uint16_t		ByteSize;
	int				FrameLastAccessed = -1;
	bool			bBreakpointed = false;
};

struct FLabelInfo : FItem
{
	FLabelInfo() { Type = ItemType::Label; }

	std::string				Name;
	bool					Global = false;
	LabelType				LabelType;
	std::map<uint16_t, int>	References;
};

struct FCodeInfo : FItem
{
	FCodeInfo() :FItem()
	{
		Type = ItemType::Code;
	}

	std::string		Text;				// Disassembly text
	uint16_t		JumpAddress = 0;	// optional jump address
	uint16_t		PointerAddress = 0;	// optional pointer address

	union
	{
		struct
		{
			bool			bDisabled : 1;
			bool			bSelfModifyingCode : 1;
		};
		uint32_t	Flags = 0;
	};
};

enum class DataType
{
	Byte,
	Word,
	Text,		// ascii text
	Graphics,	// pixel data
	Blob,		// opaque data blob
};

struct FDataInfo : FItem
{
	FDataInfo() :FItem() { Type = ItemType::Data; }

	DataType	DataType = DataType::Byte;

	int						LastFrameRead = -1;
	std::map<uint16_t, int>	Reads;	// address and counts of data access instructions
	int						LastFrameWritten = -1;
	std::map<uint16_t, int>	Writes;	// address and counts of data access instructions
};


enum class Key
{
	SetItemData,
	SetItemText,
	SetItemCode,

	AddLabel,
	Rename,
	Comment,

	Count
};

// code analysis information
struct FCodeAnalysisState
{
	ICPUInterface*			CPUInterface = nullptr;
	int						CurrentFrameNo = 0;

	static const int kAddressSize = 1 << 16;
	FLabelInfo*				Labels[kAddressSize];
	FCodeInfo*				CodeInfo[kAddressSize];
	FDataInfo*				DataInfo[kAddressSize];
	uint16_t				LastWriter[kAddressSize];
	bool					bCodeAnalysisDataDirty = false;

	bool					bRegisterDataAccesses = true;

	std::vector< FItem *>	ItemList;
	std::vector< FLabelInfo *>	GlobalDataItems;
	std::vector< FLabelInfo *>	GlobalFunctions;
	FItem*					pCursorItem = nullptr;
	int						CursorItemIndex = -1;
	int						GoToAddress = -1;
	int						HoverAddress = -1;		// address being hovered over
	int						HighlightAddress = -1;	// address to highlight
	bool					GoToLabel = false;
	std::vector<uint16_t>	AddressStack;

	int						KeyConfig[(int)Key::Count];

	std::vector< class FCommand *>	CommandStack;

};

// Commands
class FCommand
{
public:
	virtual void Do(FCodeAnalysisState &state) = 0;
	virtual void Undo(FCodeAnalysisState &state) = 0;
};


// Analysis
void InitialiseCodeAnalysis(FCodeAnalysisState &state, ICPUInterface* pCPUInterface);
bool GenerateLabelForAddress(FCodeAnalysisState &state, uint16_t pc, LabelType label);
void RunStaticCodeAnalysis(FCodeAnalysisState &state, uint16_t pc);
void RegisterCodeExecuted(FCodeAnalysisState &state, uint16_t pc);
void ReAnalyseCode(FCodeAnalysisState &state);
void GenerateGlobalInfo(FCodeAnalysisState &state);
void RegisterDataAccess(FCodeAnalysisState &state, uint16_t pc, uint16_t dataAddr, bool bWrite);
void UpdateCodeInfoForAddress(FCodeAnalysisState &state, uint16_t pc);
void ResetMemoryLogs(FCodeAnalysisState &state);

// Commands
void Undo(FCodeAnalysisState &state);

FLabelInfo* AddLabel(FCodeAnalysisState& state, uint16_t address, const char* name, LabelType type);
void AddLabelAtAddress(FCodeAnalysisState &state, uint16_t address);
void RemoveLabelAtAddress(FCodeAnalysisState &state, uint16_t address);
void SetLabelName(FCodeAnalysisState &state, FLabelInfo *pLabel, const char *pText);
void SetItemCode(FCodeAnalysisState &state, FItem *pItem);
void SetItemData(FCodeAnalysisState &state, FItem *pItem);
void SetItemText(FCodeAnalysisState &state, FItem *pItem);
void SetItemCommentText(FCodeAnalysisState &state, FItem *pItem, const char *pText);


bool OutputCodeAnalysisToTextFile(FCodeAnalysisState &state, const char *pTextFileName, uint16_t startAddr, uint16_t endAddr);