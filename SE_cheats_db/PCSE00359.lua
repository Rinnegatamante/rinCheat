-- Mind 0 (USA)
-- Credits: Slade

needs_crc32 = false

warning = "The first time you apply a two part cheat, you should apply both halves. After that, you only need to apply part one."

cur_chts = {
	{["name"]="999,999,999 Yen", ["offset"]=0x21EA4, ["file"]="data0000.bin", ["value"]=0x98967F, ["size"]=4},
	{["name"]="99,999 Skill Points", ["offset"]=0x21EA8, ["file"]="data0000.bin", ["value"]=0x1869F, ["size"]=4},
	{["name"]="Kei Takanashi Max Level", ["offset"]=0x34, ["file"]="data0000.bin", ["value"]=0x3473BC0, ["size"]=4},
	{["name"]="Kei Takanashi Max TP", ["offset"]=0x24, ["file"]="data0000.bin", ["value"]=0x0A, ["size"]=1},
	{["name"]="Kei Takanashi 9,999 LP", ["offset"]=0x18, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Kei Takanashi 9,999 Max LP", ["offset"]=0x1C, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Kei Takanashi 9,999 MP", ["offset"]=0x20, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Kei Takanashi 9,999 Max MP", ["offset"]=0x22, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Sana Chikage Max Level", ["offset"]=0x164, ["file"]="data0000.bin", ["value"]=0x3473BC0, ["size"]=4},
	{["name"]="Saka Chikage Max TP", ["offset"]=0x154, ["file"]="data0000.bin", ["value"]=0x0A, ["size"]=1},
	{["name"]="Saka Chikage 9,999 LP", ["offset"]=0x148, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Saka Chikage 9,999 Max LP", ["offset"]=0x14C, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Saka Chikage 9,999 MP", ["offset"]=0x150, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Saka Chikage 9,999 Max MP", ["offset"]=0x152, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Leo Asahina Max Level", ["offset"]=0x294, ["file"]="data0000.bin", ["value"]=0x3473BC0, ["size"]=4},
	{["name"]="Leo Asahina Max TP", ["offset"]=0x284, ["file"]="data0000.bin", ["value"]=0x0A, ["size"]=1},
	{["name"]="Leo Asahina 9,999 LP", ["offset"]=0x278, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Leo Asahina 9,999 Max LP", ["offset"]=0x27C, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Leo Asahina 9,999 MP", ["offset"]=0x280, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Leo Asahina 9,999 Max MP", ["offset"]=0x282, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Yoichi Ogata Max Level", ["offset"]=0x3C4, ["file"]="data0000.bin", ["value"]=0x3473BC0, ["size"]=4},
	{["name"]="Yoichi Ogata Max TP", ["offset"]=0x3B4, ["file"]="data0000.bin", ["value"]=0x0A, ["size"]=1},
	{["name"]="Yoichi Ogata 9,999 LP", ["offset"]=0x3A8, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Yoichi Ogata 9,999 Max LP", ["offset"]=0x3AC, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Yoichi Ogata 9,999 MP", ["offset"]=0x3B0, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Yoichi Ogata 9,999 Max MP", ["offset"]=0x3B2, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Kotone Siragiku Max Level", ["offset"]=0x4F4, ["file"]="data0000.bin", ["value"]=0x3473BC0, ["size"]=4},
	{["name"]="Kotone Siragiku Max TP", ["offset"]=0x4E4, ["file"]="data0000.bin", ["value"]=0x0A, ["size"]=1},
	{["name"]="Kotone Siragiku 9,999 LP", ["offset"]=0x4D8, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Kotone Siragiku 9,999 Max LP", ["offset"]=0x4DC, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Kotone Siragiku 9,999 MP", ["offset"]=0x4E0, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Kotone Siragiku 9,999 Max MP", ["offset"]=0x4E2, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Lina Albertine Max Level", ["offset"]=0x624, ["file"]="data0000.bin", ["value"]=0x3473BC0, ["size"]=4},
	{["name"]="Lina Albertine Max TP", ["offset"]=0x614, ["file"]="data0000.bin", ["value"]=0x0A, ["size"]=1},
	{["name"]="Lina Albertine 9,999 LP", ["offset"]=0x608, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Lina Albertine 9,999 Max LP", ["offset"]=0x60C, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Lina Albertine 9,999 MP", ["offset"]=0x610, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2},
	{["name"]="Lina Albertine 9,999 Max MP", ["offset"]=0x612, ["file"]="data0000.bin", ["value"]=0x270F, ["size"]=2}
}