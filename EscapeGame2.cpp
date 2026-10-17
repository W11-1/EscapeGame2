// EscapeGame2.cpp: 定义应用程序的入口点。

#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_set>
#include <vector>

#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif

namespace
{
constexpr int kScreenWidth = 1000;
constexpr int kScreenHeight = 700;
constexpr float kPlayerSpeed = 4.0f;

const Color kInk = { 25, 23, 28, 255 };
const Color kPaper = { 230, 217, 180, 255 };
const Color kChalk = { 218, 226, 213, 255 };
const Color kBlood = { 164, 43, 48, 255 };
const Color kHealth = { 205, 67, 72, 255 };
const Color kSanity = { 85, 157, 190, 255 };
const Color kTrust = { 208, 164, 72, 255 };
const Color kDesk = { 104, 70, 48, 255 };
const Color kDeskEdge = { 57, 39, 31, 255 };
const Color kFloorA = { 62, 55, 54, 255 };
const Color kFloorB = { 72, 63, 60, 255 };
const Color kWall = { 49, 54, 50, 255 };
const Color kBoard = { 26, 61, 49, 255 };

struct LocalizedText
{
    const char* zh;
    const char* en;
};

struct DialogueLine
{
    LocalizedText speaker;
    LocalizedText text;
};

struct ChoiceOption
{
    LocalizedText label;
    int healthDelta;
    int sanityDelta;
    int trustDelta;
    const DialogueLine* response;
    int responseCount;
};

enum class Phase
{
    Opening,
    Exploring,
    Dialogue,
    Choices,
    ChapterComplete,
    GameOver
};

enum class ChapterId
{
    One,
    Two,
    Three
};

enum class SceneId
{
    ClassroomOne,
    ExamRoomTwo,
    EscapeHallTwo,
    ChapterThreePlaceholder
};

enum class ChapterTwoChoiceSet
{
    None,
    Exam,
    Cheating
};

enum class DialogueOutcome
{
    ResumeExploring,
    BeginChoices,
    FinishChapter,
    BeginChapterTwoExamChoices,
    BeginChapterTwoCheatChoices,
    BeginDemonTransformation,
    StartEscape,
    FinishChapterTwo
};

enum class InteractableKind
{
    Blackboard,
    Desk,
    Door,
    Teacher,
    ExamPaperTwo,
    BlackboardTwo,
    StudentTwo,
    TeacherTwo,
    EscapeDoorTwo
};

struct Interactable
{
    InteractableKind kind;
    LocalizedText name;
    Vector2 point;
    float radius;
    bool investigated;
};

struct DialogueState
{
    const DialogueLine* lines = nullptr;
    int count = 0;
    int index = 0;
    DialogueOutcome outcome = DialogueOutcome::ResumeExploring;
};

struct GameState
{
    Vector2 player = { 90.0f, 630.0f };
    Vector2 demon = { 840.0f, 350.0f };
    float demonSpeed = 1.5f;
    float health = 100.0f;
    float sanity = 100.0f;
    float trust = 50.0f;
    float demonDamageTimer = 0.0f;
    bool demonChaseEnabled = false;
    bool teacherChoiceMade = false;
    bool chapterOneComplete = false;
    bool chapterPromptDismissed = false;
    bool chapterTwoPaperFound = false;
    bool chapterTwoBlackboardFound = false;
    bool chapterTwoStudentSpoken = false;
    bool chapterTwoExamChoiceMade = false;
    bool chapterTwoCheatChoiceMade = false;
    bool chapterTwoContractTriggered = false;
    bool teacherTransformed = false;
    bool chapterTwoComplete = false;
    Phase phase = Phase::Opening;
    DialogueState dialogue;
    int selectedChoice = 0;
    int investigatedCount = 0;
    ChapterId currentChapter = ChapterId::One;
    SceneId currentScene = SceneId::ClassroomOne;
    ChapterTwoChoiceSet chapterTwoChoiceSet = ChapterTwoChoiceSet::None;
    int chapterTwoSelectedChoice = 0;
};

template <typename T, std::size_t N>
constexpr int CountOf(T (&)[N])
{
    return static_cast<int>(N);
}

const LocalizedText kUiTexts[] = {
    { "异界逃生：考试倒计时", "Escape: Exam Countdown" },
    { "生命", "Health" },
    { "精神", "Sanity" },
    { "信任", "Trust" },
    { "WASD/方向键移动  E 调查", "WASD/Arrows: Move  E: Investigate" },
    { "E/Enter 继续", "E/Enter: Continue" },
    { "选择：1/2/3 或 W/S + E", "Choose: 1/2/3 or W/S + E" },
    { "第一章完成", "Chapter One Complete" },
    { "即将进入第二章", "Chapter Two is coming" },
    { "按 Enter 继续观察教室", "Press Enter to remain in the classroom" },
    { "调查已完成", "Investigated" },
    { "今日课程：数学 · 语言 · 历史", "TODAY: MATH · LANGUAGE · HISTORY" },
    { "门", "DOOR" },
    { "E 调查：", "E Investigate: " },
    { "中文字体加载失败，已切换英文界面。", "Chinese font failed to load. English fallback active." },
    { "ALIEN ESCAPE", "ALIEN ESCAPE" },
    { "关闭窗口以退出", "Close the window to exit" },
    { "第二章：不公平的考试", "Chapter Two: The Unfair Exam" },
    { "第二章完成", "Chapter Two Complete" },
    { "第三章：妈妈的救援即将开始", "Chapter Three: Mom's Rescue is coming" },
    { "A老师", "Teacher A" },
    { "诡异试卷", "Strange exam paper" },
    { "第二章黑板", "Chapter Two blackboard" },
    { "附近学生", "Nearby student" },
    { "逃生出口", "Escape exit" },
    { "按 Enter 进入第三章入口", "Press Enter for the Chapter Three entrance" },
    { "第三章：妈妈的救援", "Chapter Three: Mom's Rescue" },
    { "第三章内容将在后续开发。", "Chapter Three will be developed later." }
};

constexpr int kUiChapterTwoTitle = 17;
constexpr int kUiChapterTwoComplete = 18;
constexpr int kUiChapterThreeComing = 19;
constexpr int kUiTeacherA = 20;
constexpr int kUiExamPaperTwo = 21;
constexpr int kUiBlackboardTwo = 22;
constexpr int kUiStudentTwo = 23;
constexpr int kUiEscapeDoorTwo = 24;
constexpr int kUiChapterThreePrompt = 25;
constexpr int kUiChapterThreeTitle = 26;
constexpr int kUiChapterThreePlaceholder = 27;

const DialogueLine kOpeningLines[] = {
    { { "旁白", "Narrator" }, { "你只记得妈妈把一把钥匙放进你的口袋。再回过神时，你已经站在一间陌生的教室里。", "You only remember Mom placing a key in your pocket. The next moment, you are standing in a strange classroom." } },
    { { "你", "You" }, { "周围的学生长着角、鳞片和不像人类的影子，可他们说的话，你竟然全都听得懂。", "The students around you have horns, scales, and inhuman shadows, yet you understand every word they say." } },
    { { "旁白", "Narrator" }, { "这里是一所恶魔学校。你不知道自己为什么能进来，也不知道为什么没有被赶出去。", "This is a demon school. You do not know how you entered, or why no one has driven you out." } },
    { { "A老师", "Teacher A" }, { "别站着了，孩子。今天的课快开始了。", "Don't just stand there, child. Today's lesson is about to begin." } },
    { { "旁白", "Narrator" }, { "先用 E 观察黑板、邻座学生和口袋里的钥匙，再去讲台找老师。", "Use E to examine the blackboard, the neighboring student, and the key in your pocket, then visit the teacher's desk." } }
};

const DialogueLine kBlackboardLines[] = {
    { { "旁白", "Narrator" }, { "黑板上排着数学、语言和历史课表。粉笔字像人类的文字，却多出几道扭曲的笔画。", "The blackboard lists mathematics, language, and history. The letters resemble human writing, but add several twisted strokes." } },
    { { "你", "You" }, { "我盯着其中一行，那些陌生笔画竟自动变成了我能读懂的语言。", "I stare at one line, and the unfamiliar strokes rearrange themselves into words I can read." } }
};

const DialogueLine kDeskLines[] = {
    { { "旁白", "Narrator" }, { "邻座学生把试卷收进抽屉。它的手指覆盖着细小鳞片，声音却很温和。", "The neighboring student puts a test paper into a drawer. Its fingers are covered in small scales, but its voice is gentle." } },
    { { "你", "You" }, { "它用通用语向我借橡皮。我能听懂恶魔的语言，而它似乎也没有觉得我哪里奇怪。", "It asks to borrow my eraser in the common language. I can understand demon speech, and it does not seem to find me strange." } }
};

const DialogueLine kDoorLines[] = {
    { { "旁白", "Narrator" }, { "你摸到口袋里那把妈妈交给你的钥匙。钥匙没有插进任何锁，却微微发热。", "You touch the key Mom gave you. It is not inside any lock, yet it feels warm." } },
    { { "你", "You" }, { "钥匙上的纹路像门外的影子一样缓慢游动。我看不出它有什么用途，也不记得自己是怎么来到这里的。", "The patterns on the key shift slowly like shadows beyond the door. I cannot tell what it is for, or how I arrived here." } }
};

const DialogueLine kTeacherIntroLines[] = {
    { { "A老师", "Teacher A" }, { "别怕，孩子。你一直是我照顾的学生，在这里也一样。", "Don't be afraid, child. You have always been my student, and that remains true here." } },
    { { "旁白", "Narrator" }, { "教室门忽然打开，B老师抱着一叠试卷走了进来。", "The classroom door opens. Teacher B enters carrying a stack of exam papers." } },
    { { "B老师", "Teacher B" }, { "今天临时加一场考试。题目很普通，按考场规定完成即可。", "There will be an additional exam today. The questions are ordinary. Simply follow the examination rules." } },
    { { "旁白", "Narrator" }, { "B老师把试卷放在讲台上，目光在你的口袋处短暂停留。", "Teacher B places the papers on the desk, his gaze lingering briefly on your pocket." } }
};

const DialogueLine kChoiceAResponse[] = {
    { { "A老师", "Teacher A" }, { "不要让任何人看见那把钥匙。先坐回座位，我会看着你。", "Do not let anyone see that key. Return to your seat. I will watch over you." } },
    { { "旁白", "Narrator" }, { "A老师轻轻点头，可钥匙在你口袋里变得更加灼热，你的太阳穴也跟着刺痛。", "Teacher A gives a small nod, but the key grows hotter in your pocket, and pain pulses through your temples." } }
};

const DialogueLine kChoiceBResponse[] = {
    { { "B老师", "Teacher B" }, { "你回答问题的方式，不像这里的学生。", "The way you answer does not resemble this school's students." } },
    { { "旁白", "Narrator" }, { "你努力装作普通学生，周围几名恶魔学生却同时转过头来。", "You try to act like an ordinary student, but several demon students turn toward you at once." } }
};

const DialogueLine kChoiceCResponse[] = {
    { { "你", "You" }, { "B老师，这里的考试有什么规则？", "Teacher B, what are the rules of this exam?" } },
    { { "B老师", "Teacher B" }, { "规则写在黑板上。至于你能不能遵守……那是另一场考试。", "The rules are on the blackboard. Whether you can obey them is another exam entirely." } },
    { { "旁白", "Narrator" }, { "试卷边缘渗出一滴黑墨，你的手背突然一阵刺痛。", "A drop of black ink seeps from the paper's edge, and pain suddenly stings your hand." } }
};

const DialogueLine kTeacherAfterChoiceLines[] = {
    { { "A老师", "Teacher A" }, { "在恶魔学校，表现得普通一些。口袋里的东西先不要给任何人看。", "In the demon school, act ordinary. Do not show what is in your pocket to anyone." } }
};

const ChoiceOption kChoices[] = {
    { { "向 A 老师投去求助的目光", "Look to Teacher A for help" }, 0, -5, 15, kChoiceAResponse, CountOf(kChoiceAResponse) },
    { { "保持镇定，装作普通学生", "Stay calm and pretend to be an ordinary student" }, 0, 5, -10, kChoiceBResponse, CountOf(kChoiceBResponse) },
    { { "询问 B 老师考试规则", "Ask Teacher B about the examination rules" }, -5, -3, -15, kChoiceCResponse, CountOf(kChoiceCResponse) }
};

const DialogueLine kChapterTwoOpeningLines[] = {
    { { "旁白", "Narrator" }, { "第一章结束后，教室的钟声忽然停止。", "After Chapter One, the classroom clock falls silent." } },
    { { "旁白", "Narrator" }, { "你被带到另一间考场，门上写着：不公平的考试。", "You are taken to another exam room. The sign reads: The Unfair Exam." } },
    { { "B老师", "Teacher B" }, { "这场考试没有标准答案。答错的人，会被记入契约。", "This exam has no standard answer. The wrong answer is written into a contract." } },
    { { "旁白", "Narrator" }, { "调查诡异试卷和黑板，再观察附近学生。", "Investigate the strange paper and blackboard, then observe the nearby student." } }
};

const DialogueLine kChapterTwoPaperLines[] = {
    { { "旁白", "Narrator" }, { "试卷没有姓名栏，只有一道题：当规则背叛你时，你会背叛谁？", "The paper has no name field, only one question: When the rules betray you, whom will you betray?" } },
    { { "你", "You" }, { "纸面渗出黑色墨水。答案栏里已经写着一个名字：妈妈。", "Black ink seeps through the page. One name is already written there: Mom." } }
};

const DialogueLine kChapterTwoBlackboardLines[] = {
    { { "旁白", "Narrator" }, { "黑板上没有考试范围，只有一条规则：学生不得质疑老师。", "There is no exam scope on the blackboard, only one rule: students may not question the teacher." } },
    { { "你", "You" }, { "倒计时没有数字，只有一只缓慢睁开的眼睛。", "The countdown has no numbers, only an eye slowly opening." } }
};

const DialogueLine kChapterTwoStudentPreludeLines[] = {
    { { "附近学生", "Nearby student" }, { "B老师刚才把一张纸条塞进了别人的试卷。", "Teacher B just slipped a note into someone else's paper." } },
    { { "旁白", "Narrator" }, { "先调查诡异试卷和黑板，再回来观察这名学生。", "Investigate the strange paper and blackboard, then return to this student." } }
};

const DialogueLine kChapterTwoCheatIntroLines[] = {
    { { "旁白", "Narrator" }, { "考试进行到一半，B老师俯身靠近前排学生。", "Halfway through the exam, Teacher B leans toward a student in the front row." } },
    { { "附近学生", "Nearby student" }, { "别看我……老师说只要保持沉默，成绩就不会被抹掉。", "Don't look at me... The teacher said my score stays if I remain silent." } },
    { { "B老师", "Teacher B" }, { "专心答题。帮助同学，也是我的职责。", "Keep answering. Helping a student is part of my duty." } },
    { { "旁白", "Narrator" }, { "你看见了那张被调换的答案纸。", "You see the answer sheet that has been swapped." } }
};

const DialogueLine kChapterTwoExamAnswerResponse[] = {
    { { "B老师", "Teacher B" }, { "至少你敢写下答案。契约会记住这一点。", "At least you dared to write an answer. The contract will remember that." } },
    { { "旁白", "Narrator" }, { "笔尖划破纸张，你的精神被抽走了一部分，但 B 老师认可了你的回答。", "The pen tears the paper. Part of your mind is pulled away, but Teacher B accepts your answer." } }
};

const DialogueLine kChapterTwoExamSkipResponse[] = {
    { { "旁白", "Narrator" }, { "你跳过题目。空白答案栏里慢慢浮现出一道血痕。", "You skip the question. A trail of blood slowly appears in the blank answer field." } },
    { { "B老师", "Teacher B" }, { "不回答，也是一种回答。你的犹豫已经被记录。", "Silence is also an answer. Your hesitation has been recorded." } }
};

const DialogueLine kChapterTwoExamQuestionResponse[] = {
    { { "B老师", "Teacher B" }, { "质疑规则？很好，规则也正在观察你。", "Questioning the rules? Good. The rules are watching you as well." } },
    { { "旁白", "Narrator" }, { "黑板上的眼睛转向了你，教室里所有人同时停止呼吸。", "The eye on the blackboard turns toward you. Everyone in the room stops breathing." } }
};

const DialogueLine kChapterTwoChoiceSilenceResponse[] = {
    { { "旁白", "Narrator" }, { "你低下头保持沉默。B老师满意地点了点那张被调换的答案纸。", "You lower your head and stay silent. Teacher B nods approvingly at the swapped answer sheet." } },
    { { "B老师", "Teacher B" }, { "聪明的学生知道什么时候看不见。", "A clever student knows when not to see." } }
};

const DialogueLine kChapterTwoChoiceObserveResponse[] = {
    { { "旁白", "Narrator" }, { "你假装看题，暗中记下 B 老师换卷的顺序。", "You pretend to read the test and secretly memorize the order in which Teacher B swaps the papers." } },
    { { "你", "You" }, { "纸条上的字不是答案，而是一串契约编号。", "The writing on the note is not an answer, but a contract number." } }
};

const DialogueLine kChapterTwoChoiceExposeResponse[] = {
    { { "你", "You" }, { "B老师，你在帮学生作弊。", "Teacher B, you are helping the student cheat." } },
    { { "B老师", "Teacher B" }, { "揭穿监考老师，会让整个考场一起受罚。", "Exposing the invigilator makes the entire room pay the price." } },
    { { "旁白", "Narrator" }, { "戒尺抽在你的手背上，教室里的眼睛齐齐睁开。", "A ruler strikes your hand. Every eye in the room opens at once." } }
};

const DialogueLine kChapterTwoContractLines[] = {
    { { "A老师", "Teacher A" }, { "考试结束，所有人交卷。", "The exam is over. Everyone, hand in your papers." } },
    { { "旁白", "Narrator" }, { "试卷上的名字开始燃烧，B老师脚下的影子长出第二张脸。", "The names on the papers ignite. A second face grows from Teacher B's shadow." } },
    { { "B老师", "Teacher B" }, { "契约已经生效。现在，逃吧。", "The contract is active. Now run." } },
    { { "旁白", "Narrator" }, { "B老师撕开人形外壳，恶魔从试卷堆中站了起来。", "Teacher B tears away his human shell. A demon rises from the pile of exam papers." } }
};

const DialogueLine kChapterTwoEscapeSuccessLines[] = {
    { { "旁白", "Narrator" }, { "你撞开走廊尽头的门，身后的契约发出尖啸。", "You break through the door at the end of the corridor as the contract shrieks behind you." } },
    { { "你", "You" }, { "走廊尽头传来妈妈的声音。", "Your mother's voice comes from beyond the corridor." } }
};

const ChoiceOption kChapterTwoExamChoices[] = {
    { { "认真写出自己的答案", "Write your own answer carefully" }, 0, -4, 6, kChapterTwoExamAnswerResponse, CountOf(kChapterTwoExamAnswerResponse) },
    { { "跳过这道异常题目", "Skip the abnormal question" }, 0, 2, -5, kChapterTwoExamSkipResponse, CountOf(kChapterTwoExamSkipResponse) },
    { { "质疑题目与考试规则", "Question the question and the exam rules" }, 0, -6, -10, kChapterTwoExamQuestionResponse, CountOf(kChapterTwoExamQuestionResponse) }
};

const ChoiceOption kChapterTwoCheatChoices[] = {
    { { "保持沉默，当作没有看见", "Stay silent and pretend not to see" }, 0, -8, 12, kChapterTwoChoiceSilenceResponse, CountOf(kChapterTwoChoiceSilenceResponse) },
    { { "暗中观察，记住换卷顺序", "Observe secretly and memorize the paper swaps" }, 0, -4, -5, kChapterTwoChoiceObserveResponse, CountOf(kChapterTwoChoiceObserveResponse) },
    { { "当面揭穿 B 老师作弊", "Expose Teacher B's cheating directly" }, -5, 4, -15, kChapterTwoChoiceExposeResponse, CountOf(kChapterTwoChoiceExposeResponse) }
};

const char* Text(const LocalizedText& text, bool chineseFontLoaded)
{
    return chineseFontLoaded ? text.zh : text.en;
}

void AddTextCodepoints(const char* text, std::vector<int>& codepoints, std::unordered_set<int>& seen)
{
    const char* cursor = text;

    while (cursor != nullptr && *cursor != '\0')
    {
        int codepointSize = 0;
        const int codepoint = GetCodepointNext(cursor, &codepointSize);

        if (codepointSize <= 0)
        {
            break;
        }

        if (codepoint > 0 && seen.insert(codepoint).second)
        {
            codepoints.push_back(codepoint);
        }

        cursor += codepointSize;
    }
}

void AddLineCodepoints(const DialogueLine* lines, int count, std::vector<int>& codepoints, std::unordered_set<int>& seen)
{
    for (int i = 0; i < count; ++i)
    {
        AddTextCodepoints(lines[i].speaker.zh, codepoints, seen);
        AddTextCodepoints(lines[i].speaker.en, codepoints, seen);
        AddTextCodepoints(lines[i].text.zh, codepoints, seen);
        AddTextCodepoints(lines[i].text.en, codepoints, seen);
    }
}

Font LoadGameFont(bool& chineseFontLoaded)
{
    chineseFontLoaded = false;

    std::vector<int> codepoints;
    std::unordered_set<int> seen;

    AddTextCodepoints(
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz /:.,!?()[]<>+-%",
        codepoints,
        seen);

    for (const LocalizedText& text : kUiTexts)
    {
        AddTextCodepoints(text.zh, codepoints, seen);
        AddTextCodepoints(text.en, codepoints, seen);
    }

    AddLineCodepoints(kOpeningLines, CountOf(kOpeningLines), codepoints, seen);
    AddLineCodepoints(kBlackboardLines, CountOf(kBlackboardLines), codepoints, seen);
    AddLineCodepoints(kDeskLines, CountOf(kDeskLines), codepoints, seen);
    AddLineCodepoints(kDoorLines, CountOf(kDoorLines), codepoints, seen);
    AddLineCodepoints(kTeacherIntroLines, CountOf(kTeacherIntroLines), codepoints, seen);
    AddLineCodepoints(kChoiceAResponse, CountOf(kChoiceAResponse), codepoints, seen);
    AddLineCodepoints(kChoiceBResponse, CountOf(kChoiceBResponse), codepoints, seen);
    AddLineCodepoints(kChoiceCResponse, CountOf(kChoiceCResponse), codepoints, seen);
    AddLineCodepoints(kTeacherAfterChoiceLines, CountOf(kTeacherAfterChoiceLines), codepoints, seen);
    AddLineCodepoints(kChapterTwoOpeningLines, CountOf(kChapterTwoOpeningLines), codepoints, seen);
    AddLineCodepoints(kChapterTwoPaperLines, CountOf(kChapterTwoPaperLines), codepoints, seen);
    AddLineCodepoints(kChapterTwoBlackboardLines, CountOf(kChapterTwoBlackboardLines), codepoints, seen);
    AddLineCodepoints(kChapterTwoStudentPreludeLines, CountOf(kChapterTwoStudentPreludeLines), codepoints, seen);
    AddLineCodepoints(kChapterTwoCheatIntroLines, CountOf(kChapterTwoCheatIntroLines), codepoints, seen);
    AddLineCodepoints(kChapterTwoExamAnswerResponse, CountOf(kChapterTwoExamAnswerResponse), codepoints, seen);
    AddLineCodepoints(kChapterTwoExamSkipResponse, CountOf(kChapterTwoExamSkipResponse), codepoints, seen);
    AddLineCodepoints(kChapterTwoExamQuestionResponse, CountOf(kChapterTwoExamQuestionResponse), codepoints, seen);
    AddLineCodepoints(kChapterTwoChoiceSilenceResponse, CountOf(kChapterTwoChoiceSilenceResponse), codepoints, seen);
    AddLineCodepoints(kChapterTwoChoiceObserveResponse, CountOf(kChapterTwoChoiceObserveResponse), codepoints, seen);
    AddLineCodepoints(kChapterTwoChoiceExposeResponse, CountOf(kChapterTwoChoiceExposeResponse), codepoints, seen);
    AddLineCodepoints(kChapterTwoContractLines, CountOf(kChapterTwoContractLines), codepoints, seen);
    AddLineCodepoints(kChapterTwoEscapeSuccessLines, CountOf(kChapterTwoEscapeSuccessLines), codepoints, seen);

    for (const ChoiceOption& choice : kChoices)
    {
        AddTextCodepoints(choice.label.zh, codepoints, seen);
        AddTextCodepoints(choice.label.en, codepoints, seen);
    }

    for (const ChoiceOption& choice : kChapterTwoExamChoices)
    {
        AddTextCodepoints(choice.label.zh, codepoints, seen);
        AddTextCodepoints(choice.label.en, codepoints, seen);
    }

    for (const ChoiceOption& choice : kChapterTwoCheatChoices)
    {
        AddTextCodepoints(choice.label.zh, codepoints, seen);
        AddTextCodepoints(choice.label.en, codepoints, seen);
    }

    const char* fontPaths[] = {
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/msyh.ttc"
    };

    for (const char* fontPath : fontPaths)
    {
        if (!FileExists(fontPath))
        {
            continue;
        }

        Font font = LoadFontEx(fontPath, 40, codepoints.data(), static_cast<int>(codepoints.size()));
        if (font.texture.id != 0 && font.glyphCount > 0)
        {
            chineseFontLoaded = true;
            SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
            return font;
        }
    }

    TraceLog(LOG_WARNING, "Chinese font loading failed; English fallback is active.");
    return GetFontDefault();
}

void DrawTextLocalized(
    Font font,
    const LocalizedText& text,
    Vector2 position,
    float fontSize,
    Color color,
    bool chineseFontLoaded)
{
    DrawTextEx(font, Text(text, chineseFontLoaded), position, fontSize, 1.0f, color);
}

void DrawTextCentered(
    Font font,
    const char* text,
    float centerX,
    float y,
    float fontSize,
    Color color)
{
    const Vector2 size = MeasureTextEx(font, text, fontSize, 1.0f);
    DrawTextEx(font, text, { centerX - size.x * 0.5f, y }, fontSize, 1.0f, color);
}

std::vector<std::string> WrapText(
    Font font,
    const std::string& text,
    float fontSize,
    float maxWidth)
{
    std::vector<std::string> lines;
    std::string current;
    const char* cursor = text.c_str();

    while (*cursor != '\0')
    {
        int codepointSize = 0;
        const int codepoint = GetCodepointNext(cursor, &codepointSize);
        if (codepointSize <= 0)
        {
            break;
        }

        if (codepoint == '\n')
        {
            lines.push_back(current);
            current.clear();
            cursor += codepointSize;
            continue;
        }

        std::string glyph(cursor, codepointSize);
        const std::string candidate = current + glyph;
        const float width = MeasureTextEx(font, candidate.c_str(), fontSize, 1.0f).x;

        if (!current.empty() && width > maxWidth)
        {
            lines.push_back(current);
            current = glyph;
        }
        else
        {
            current = candidate;
        }

        cursor += codepointSize;
    }

    if (!current.empty())
    {
        lines.push_back(current);
    }

    return lines;
}

void DrawWrappedText(
    Font font,
    const char* text,
    Vector2 position,
    float fontSize,
    float maxWidth,
    float lineHeight,
    Color color)
{
    const std::vector<std::string> lines = WrapText(font, text, fontSize, maxWidth);

    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        DrawTextEx(
            font,
            lines[i].c_str(),
            { position.x, position.y + static_cast<float>(i) * lineHeight },
            fontSize,
            1.0f,
            color);
    }
}

void ApplyStatDelta(GameState& state, int healthDelta, int sanityDelta, int trustDelta)
{
    state.health = Clamp(state.health + static_cast<float>(healthDelta), 0.0f, 100.0f);
    state.sanity = Clamp(state.sanity + static_cast<float>(sanityDelta), 0.0f, 100.0f);
    state.trust = Clamp(state.trust + static_cast<float>(trustDelta), 0.0f, 100.0f);
}

void StartDialogue(
    GameState& state,
    const DialogueLine* lines,
    int count,
    DialogueOutcome outcome)
{
    state.dialogue.lines = lines;
    state.dialogue.count = count;
    state.dialogue.index = 0;
    state.dialogue.outcome = outcome;
    state.phase = Phase::Dialogue;
}

void EnterChapterTwo(GameState& state)
{
    if (state.currentChapter == ChapterId::Two)
    {
        return;
    }

    state.currentChapter = ChapterId::Two;
    state.currentScene = SceneId::ExamRoomTwo;
    state.player = { 90.0f, 630.0f };
    state.demon = { 755.0f, 330.0f };
    state.demonSpeed = 2.2f;
    state.demonDamageTimer = 0.0f;
    state.demonChaseEnabled = false;
    state.chapterTwoPaperFound = false;
    state.chapterTwoBlackboardFound = false;
    state.chapterTwoStudentSpoken = false;
    state.chapterTwoExamChoiceMade = false;
    state.chapterTwoCheatChoiceMade = false;
    state.chapterTwoContractTriggered = false;
    state.teacherTransformed = false;
    state.chapterTwoComplete = false;
    state.chapterTwoChoiceSet = ChapterTwoChoiceSet::None;
    state.chapterTwoSelectedChoice = 0;
    state.chapterPromptDismissed = true;

    StartDialogue(
        state,
        kChapterTwoOpeningLines,
        CountOf(kChapterTwoOpeningLines),
        DialogueOutcome::ResumeExploring);
}

void BeginChapterTwoTransformation(GameState& state)
{
    if (state.chapterTwoContractTriggered)
    {
        return;
    }

    state.chapterTwoContractTriggered = true;
    state.teacherTransformed = true;
    state.demonChaseEnabled = false;
    state.demon = { 755.0f, 320.0f };
    state.demonDamageTimer = 0.0f;

    StartDialogue(
        state,
        kChapterTwoContractLines,
        CountOf(kChapterTwoContractLines),
        DialogueOutcome::StartEscape);
}

void StartChapterTwoEscape(GameState& state)
{
    state.currentScene = SceneId::EscapeHallTwo;
    state.player = { 100.0f, 610.0f };
    state.demon = { 760.0f, 220.0f };
    state.demonSpeed = 2.6f;
    state.demonDamageTimer = 0.0f;
    state.demonChaseEnabled = true;
    state.phase = Phase::Exploring;
}

bool AdvanceDialogue(GameState& state)
{
    if (state.dialogue.lines == nullptr || state.dialogue.count <= 0)
    {
        state.phase = Phase::Exploring;
        return true;
    }

    state.dialogue.index++;
    if (state.dialogue.index < state.dialogue.count)
    {
        return true;
    }

    switch (state.dialogue.outcome)
    {
        case DialogueOutcome::ResumeExploring:
            state.phase = Phase::Exploring;
            break;
        case DialogueOutcome::BeginChoices:
            state.selectedChoice = 0;
            state.phase = Phase::Choices;
            break;
        case DialogueOutcome::FinishChapter:
            state.chapterOneComplete = true;
            state.phase = Phase::ChapterComplete;
            break;
        case DialogueOutcome::BeginChapterTwoExamChoices:
            state.chapterTwoChoiceSet = ChapterTwoChoiceSet::Exam;
            state.chapterTwoSelectedChoice = 0;
            state.phase = Phase::Choices;
            break;
        case DialogueOutcome::BeginChapterTwoCheatChoices:
            state.chapterTwoChoiceSet = ChapterTwoChoiceSet::Cheating;
            state.chapterTwoSelectedChoice = 0;
            state.phase = Phase::Choices;
            break;
        case DialogueOutcome::BeginDemonTransformation:
            BeginChapterTwoTransformation(state);
            break;
        case DialogueOutcome::StartEscape:
            StartChapterTwoEscape(state);
            break;
        case DialogueOutcome::FinishChapterTwo:
            state.chapterTwoComplete = true;
            state.demonChaseEnabled = false;
            state.phase = Phase::ChapterComplete;
            break;
    }

    return true;
}

Rectangle PlayerCollisionRect(Vector2 player)
{
    return { player.x - 11.0f, player.y - 19.0f, 22.0f, 38.0f };
}

const Rectangle kPlayArea = { 72.0f, 185.0f, 856.0f, 465.0f };
const Rectangle kPodium = { 700.0f, 280.0f, 110.0f, 40.0f };
const Rectangle kDesks[] = {
    { 135.0f, 350.0f, 110.0f, 38.0f },
    { 300.0f, 350.0f, 110.0f, 38.0f },
    { 465.0f, 350.0f, 110.0f, 38.0f },
    { 630.0f, 350.0f, 110.0f, 38.0f },
    { 795.0f, 350.0f, 110.0f, 38.0f },

    { 135.0f, 465.0f, 110.0f, 38.0f },
    { 300.0f, 465.0f, 110.0f, 38.0f },
    { 465.0f, 465.0f, 110.0f, 38.0f },
    { 630.0f, 465.0f, 110.0f, 38.0f },
    { 795.0f, 465.0f, 110.0f, 38.0f },

    { 135.0f, 580.0f, 110.0f, 38.0f },
    { 300.0f, 580.0f, 110.0f, 38.0f },
    { 465.0f, 580.0f, 110.0f, 38.0f },
    { 630.0f, 580.0f, 110.0f, 38.0f },
    { 795.0f, 580.0f, 110.0f, 38.0f }
};

bool CanStandAt(Vector2 position)
{
    const Rectangle playerRect = PlayerCollisionRect(position);
    const Rectangle playAreaRect = {
        kPlayArea.x,
        kPlayArea.y,
        kPlayArea.width,
        kPlayArea.height
    };

    if (playerRect.x < playAreaRect.x ||
        playerRect.y < playAreaRect.y ||
        playerRect.x + playerRect.width > playAreaRect.x + playAreaRect.width ||
        playerRect.y + playerRect.height > playAreaRect.y + playAreaRect.height)
    {
        return false;
    }

    for (const Rectangle& desk : kDesks)
    {
        if (CheckCollisionRecs(playerRect, desk))
        {
            return false;
        }
    }

    return !CheckCollisionRecs(playerRect, kPodium);
}

const Rectangle kExamRoomTwoPlayArea = { 72.0f, 185.0f, 856.0f, 465.0f };
const Rectangle kExamRoomTwoObstacles[] = {
    { 170.0f, 365.0f, 110.0f, 38.0f },
    { 360.0f, 365.0f, 110.0f, 38.0f },
    { 550.0f, 365.0f, 110.0f, 38.0f },
    { 740.0f, 365.0f, 110.0f, 38.0f },
    { 170.0f, 485.0f, 110.0f, 38.0f },
    { 360.0f, 485.0f, 110.0f, 38.0f },
    { 550.0f, 485.0f, 110.0f, 38.0f },
    { 740.0f, 485.0f, 110.0f, 38.0f },
    { 170.0f, 585.0f, 110.0f, 38.0f },
    { 360.0f, 585.0f, 110.0f, 38.0f },
    { 550.0f, 585.0f, 110.0f, 38.0f },
    { 740.0f, 585.0f, 110.0f, 38.0f },
    { 700.0f, 275.0f, 115.0f, 40.0f }
};

const Rectangle kEscapeHallTwoPlayArea = { 72.0f, 185.0f, 856.0f, 465.0f };
const Rectangle kEscapeHallTwoObstacles[] = {
    { 165.0f, 290.0f, 135.0f, 52.0f },
    { 390.0f, 430.0f, 135.0f, 52.0f },
    { 625.0f, 285.0f, 135.0f, 52.0f },
    { 360.0f, 585.0f, 170.0f, 38.0f }
};

bool CanStandInChapterTwo(
    SceneId scene,
    Vector2 position)
{
    const Rectangle playerRect = PlayerCollisionRect(position);
    const Rectangle playArea =
        (scene == SceneId::EscapeHallTwo)
            ? kEscapeHallTwoPlayArea
            : kExamRoomTwoPlayArea;

    if (playerRect.x < playArea.x ||
        playerRect.y < playArea.y ||
        playerRect.x + playerRect.width > playArea.x + playArea.width ||
        playerRect.y + playerRect.height > playArea.y + playArea.height)
    {
        return false;
    }

    if (scene == SceneId::EscapeHallTwo)
    {
        for (const Rectangle& obstacle : kEscapeHallTwoObstacles)
        {
            if (CheckCollisionRecs(playerRect, obstacle))
            {
                return false;
            }
        }
    }
    else
    {
        for (const Rectangle& obstacle : kExamRoomTwoObstacles)
        {
            if (CheckCollisionRecs(playerRect, obstacle))
            {
                return false;
            }
        }
    }

    return true;
}

bool CanStandAtInScene(const GameState& state, Vector2 position)
{
    if (state.currentScene == SceneId::ClassroomOne)
    {
        return CanStandAt(position);
    }

    return CanStandInChapterTwo(state.currentScene, position);
}

void MovePlayer(GameState& state)
{
    Vector2 direction = { 0.0f, 0.0f };

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        direction.y -= 1.0f;

    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        direction.y += 1.0f;

    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        direction.x -= 1.0f;

    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        direction.x += 1.0f;

    if (Vector2Length(direction) <= 0.0f)
    {
        return;
    }

    direction = Vector2Normalize(direction);

    Vector2 candidate = state.player;
    candidate.x += direction.x * kPlayerSpeed;
    if (CanStandAtInScene(state, candidate))
    {
        state.player.x = candidate.x;
    }

    candidate = state.player;
    candidate.y += direction.y * kPlayerSpeed;
    if (CanStandAtInScene(state, candidate))
    {
        state.player.y = candidate.y;
    }
}

void UpdateDemonChase(GameState& state, float deltaTime)
{
    if (!state.demonChaseEnabled)
    {
        return;
    }

    Vector2 demonDirection = {
        state.player.x - state.demon.x,
        state.player.y - state.demon.y
    };

    const float distance = Vector2Length(demonDirection);

    if (distance > 1.0f)
    {
        demonDirection = Vector2Normalize(demonDirection);
        state.demon.x += demonDirection.x * state.demonSpeed;
        state.demon.y += demonDirection.y * state.demonSpeed;
    }

    if (CheckCollisionCircles(state.player, 15.0f, state.demon, 20.0f))
    {
        state.demonDamageTimer += deltaTime;

        if (state.demonDamageTimer >= 0.5f)
        {
            state.health -= 5.0f;
            state.demonDamageTimer = 0.0f;
        }
    }
    else
    {
        state.demonDamageTimer = 0.0f;
    }

    if (state.health < 0.0f)
    {
        state.health = 0.0f;
    }

    if (state.health <= 0.0f)
    {
        state.phase = Phase::GameOver;
    }
}

void DrawDesk(const Rectangle& desk)
{
    DrawRectangleRec(
        { desk.x + 8.0f, desk.y + desk.height, 10.0f, 17.0f },
        kDeskEdge);
    DrawRectangleRec(
        { desk.x + desk.width - 18.0f, desk.y + desk.height, 10.0f, 17.0f },
        kDeskEdge);
    DrawRectangleRec(
        { desk.x + desk.width * 0.5f - 22.0f, desk.y - 24.0f, 44.0f, 18.0f },
        { 72, 50, 41, 255 });
    DrawRectangleRec(
        { desk.x + desk.width * 0.5f - 18.0f, desk.y - 20.0f, 36.0f, 5.0f },
        { 129, 91, 58, 255 });
    DrawRectangleRec(desk, kDeskEdge);
    DrawRectangleRec(
        { desk.x + 3.0f, desk.y + 3.0f, desk.width - 6.0f, desk.height - 6.0f },
        kDesk);
    DrawRectangleRec(
        { desk.x + 10.0f, desk.y + 9.0f, desk.width - 20.0f, 5.0f },
        { 145, 101, 65, 255 });
}

void DrawClassroom(
    Font font,
    bool chineseFontLoaded,
    const GameState& state)
{
    ClearBackground(kInk);

    DrawRectangle(0, 0, kScreenWidth, 110, { 20, 19, 23, 255 });
    DrawRectangle(40, 108, 920, 575, kWall);
    DrawRectangle(58, 178, 884, 486, { 40, 38, 38, 255 });

    constexpr int tileWidth = 52;
    constexpr int tileHeight = 32;
    for (int row = 0; row < 15; ++row)
    {
        for (int column = 0; column < 17; ++column)
        {
            const int x = 58 + column * tileWidth;
            const int y = 178 + row * tileHeight;
            if (x >= 942 || y >= 664)
            {
                continue;
            }

            DrawRectangle(
                x,
                y,
                std::min(tileWidth, 942 - x),
                std::min(tileHeight, 664 - y),
                ((row + column) % 2 == 0) ? kFloorA : kFloorB);
        }
    }

    DrawRectangle(150, 119, 380, 63, { 32, 28, 26, 255 });
    DrawRectangle(156, 125, 368, 51, kBoard);
    DrawRectangle(164, 131, 352, 2, { 90, 115, 91, 255 });
    DrawTextLocalized(
        font,
        kUiTexts[11],
        { 170.0f, 143.0f },
        19.0f,
        kChalk,
        chineseFontLoaded);

    DrawRectangle(600, 119, 190, 79, { 30, 34, 38, 255 });
    DrawRectangle(607, 126, 176, 65, { 42, 58, 68, 255 });
    DrawRectangle(695, 126, 5, 65, { 23, 25, 28, 255 });
    DrawRectangle(607, 154, 176, 4, { 23, 25, 28, 255 });

    DrawRectangle(902, 190, 54, 132, { 48, 34, 28, 255 });
    DrawRectangle(910, 198, 38, 116, { 111, 76, 51, 255 });
    DrawCircle(938, 257, 3, { 202, 172, 82, 255 });
    DrawTextLocalized(
        font,
        kUiTexts[12],
        { 918.0f, 172.0f },
        16.0f,
        kPaper,
        chineseFontLoaded);

    for (const Rectangle& desk : kDesks)
    {
        DrawDesk(desk);
    }

    DrawRectangleRec(kPodium, kDeskEdge);
    DrawRectangleRec(
        { kPodium.x + 4.0f, kPodium.y + 4.0f, kPodium.width - 8.0f, kPodium.height - 8.0f },
        kDesk);

    const int teacherX = 755;
    const int teacherY = 245;
    DrawRectangle(teacherX - 17, teacherY - 28, 34, 13, { 63, 57, 55, 255 });
    DrawRectangle(teacherX - 11, teacherY - 18, 22, 18, { 215, 189, 158, 255 });
    DrawRectangle(teacherX - 15, teacherY - 19, 5, 9, { 63, 57, 55, 255 });
    DrawRectangle(teacherX + 10, teacherY - 19, 5, 9, { 63, 57, 55, 255 });
    DrawRectangle(teacherX - 7, teacherY - 10, 6, 2, kInk);
    DrawRectangle(teacherX + 2, teacherY - 10, 6, 2, kInk);
    DrawRectangle(teacherX - 17, teacherY, 34, 35, { 63, 73, 78, 255 });
    DrawRectangle(teacherX - 4, teacherY + 3, 8, 24, kPaper);
    DrawRectangle(teacherX + 1, teacherY + 7, 3, 3, kTrust);

    if (state.demonChaseEnabled)
    {
        DrawCircleV(state.demon, 20.0f, kBlood);
    }
}

void DrawDemonSprite(Vector2 demon)
{
    const int x = static_cast<int>(std::roundf(demon.x));
    const int y = static_cast<int>(std::roundf(demon.y));

    DrawTriangle(
        { static_cast<float>(x - 20), static_cast<float>(y - 14) },
        { static_cast<float>(x - 5), static_cast<float>(y - 40) },
        { static_cast<float>(x + 2), static_cast<float>(y - 12) },
        kBlood);
    DrawTriangle(
        { static_cast<float>(x + 20), static_cast<float>(y - 14) },
        { static_cast<float>(x + 5), static_cast<float>(y - 40) },
        { static_cast<float>(x - 2), static_cast<float>(y - 12) },
        kBlood);
    DrawCircleV(demon, 20.0f, kBlood);
    DrawRectangle(x - 11, y - 2, 7, 4, kInk);
    DrawRectangle(x + 4, y - 2, 7, 4, kInk);
    DrawRectangle(x - 7, y + 11, 14, 4, { 255, 194, 116, 255 });
}

void DrawExamRoomTwo(
    Font font,
    bool chineseFontLoaded,
    const GameState& state)
{
    ClearBackground(kInk);
    DrawRectangle(0, 0, kScreenWidth, 110, { 20, 19, 23, 255 });
    DrawRectangle(40, 108, 920, 575, { 45, 43, 42, 255 });
    DrawRectangle(58, 178, 884, 486, { 38, 35, 37, 255 });

    constexpr int tileWidth = 52;
    constexpr int tileHeight = 32;
    for (int row = 0; row < 15; ++row)
    {
        for (int column = 0; column < 17; ++column)
        {
            const int x = 58 + column * tileWidth;
            const int y = 178 + row * tileHeight;
            if (x >= 942 || y >= 664)
            {
                continue;
            }

            DrawRectangle(
                x,
                y,
                std::min(tileWidth, 942 - x),
                std::min(tileHeight, 664 - y),
                ((row + column) % 2 == 0) ? Color{ 67, 58, 55, 255 } : Color{ 57, 51, 50, 255 });
        }
    }

    DrawRectangle(105, 119, 395, 65, { 28, 25, 25, 255 });
    DrawRectangle(111, 125, 383, 53, { 42, 54, 46, 255 });
    DrawTextLocalized(
        font,
        kUiTexts[kUiChapterTwoTitle],
        { 124.0f, 141.0f },
        19.0f,
        kChalk,
        chineseFontLoaded);

    DrawRectangle(555, 119, 220, 72, { 31, 33, 39, 255 });
    DrawRectangle(562, 126, 206, 58, { 48, 52, 67, 255 });
    DrawRectangle(665, 126, 5, 58, { 24, 24, 29, 255 });
    DrawRectangle(562, 155, 206, 3, { 24, 24, 29, 255 });

    for (int i = 0; i < CountOf(kExamRoomTwoObstacles) - 1; ++i)
    {
        DrawDesk(kExamRoomTwoObstacles[i]);

        const Rectangle& desk = kExamRoomTwoObstacles[i];
        DrawRectangle(
            static_cast<int>(desk.x + desk.width * 0.5f - 8.0f),
            static_cast<int>(desk.y - 46.0f),
            16,
            13,
            { 201, 176, 148, 255 });
        DrawRectangle(
            static_cast<int>(desk.x + desk.width * 0.5f - 10.0f),
            static_cast<int>(desk.y - 34.0f),
            20,
            9,
            { 48, 65, 78, 255 });
    }

    DrawRectangle(365, 351, 105, 18, kPaper);
    DrawRectangle(373, 356, 88, 3, kBlood);
    DrawRectangle(373, 362, 57, 2, { 73, 66, 62, 255 });

    const Rectangle& podium = kExamRoomTwoObstacles[CountOf(kExamRoomTwoObstacles) - 1];
    DrawRectangleRec(podium, kDeskEdge);
    DrawRectangleRec(
        { podium.x + 4.0f, podium.y + 4.0f, podium.width - 8.0f, podium.height - 8.0f },
        kDesk);

    DrawRectangle(235, 211, 32, 12, { 53, 47, 48, 255 });
    DrawRectangle(241, 221, 20, 18, { 214, 190, 163, 255 });
    DrawRectangle(227, 239, 48, 35, { 76, 69, 72, 255 });
    DrawRectangle(247, 244, 8, 24, kPaper);

    if (state.teacherTransformed)
    {
        DrawDemonSprite(state.demon);
    }
    else
    {
        DrawRectangle(738, 207, 34, 13, { 63, 57, 55, 255 });
        DrawRectangle(744, 217, 22, 18, { 215, 189, 158, 255 });
        DrawRectangle(731, 234, 46, 35, { 63, 73, 78, 255 });
        DrawRectangle(748, 239, 8, 24, kPaper);
        DrawRectangle(755, 243, 3, 3, kTrust);
    }

    DrawRectangle(72, 172, 856, 6, { 105, 79, 55, 255 });
    DrawTextLocalized(
        font,
        kUiTexts[kUiTeacherA],
        { 222.0f, 184.0f },
        14.0f,
        kPaper,
        chineseFontLoaded);
}

void DrawEscapeHallTwo(
    Font font,
    bool chineseFontLoaded,
    const GameState& state)
{
    ClearBackground(kInk);
    DrawRectangle(0, 0, kScreenWidth, 110, { 18, 17, 20, 255 });
    DrawRectangle(40, 108, 920, 575, { 35, 38, 39, 255 });
    DrawRectangle(58, 178, 884, 486, { 29, 31, 32, 255 });

    constexpr int tileWidth = 52;
    constexpr int tileHeight = 32;
    for (int row = 0; row < 15; ++row)
    {
        for (int column = 0; column < 17; ++column)
        {
            const int x = 58 + column * tileWidth;
            const int y = 178 + row * tileHeight;
            if (x >= 942 || y >= 664)
            {
                continue;
            }

            DrawRectangle(
                x,
                y,
                std::min(tileWidth, 942 - x),
                std::min(tileHeight, 664 - y),
                ((row + column) % 2 == 0) ? Color{ 49, 47, 45, 255 } : Color{ 43, 42, 41, 255 });
        }
    }

    for (const Rectangle& obstacle : kEscapeHallTwoObstacles)
    {
        DrawRectangleRec(obstacle, { 68, 48, 38, 255 });
        DrawRectangleLinesEx(obstacle, 3.0f, { 126, 88, 55, 255 });
        DrawRectangle(
            static_cast<int>(obstacle.x + obstacle.width * 0.5f - 2.0f),
            static_cast<int>(obstacle.y + 8.0f),
            4,
            static_cast<int>(obstacle.height - 16.0f),
            { 30, 28, 27, 255 });
    }

    DrawRectangle(902, 190, 52, 135, { 34, 26, 24, 255 });
    DrawRectangle(909, 198, 38, 119, { 103, 69, 48, 255 });
    DrawRectangle(918, 211, 20, 56, { 53, 38, 30, 255 });
    DrawCircle(938, 260, 4, kTrust);
    DrawTextLocalized(
        font,
        kUiTexts[kUiEscapeDoorTwo],
        { 886.0f, 168.0f },
        15.0f,
        kPaper,
        chineseFontLoaded);

    if (state.demonChaseEnabled || state.teacherTransformed)
    {
        DrawDemonSprite(state.demon);
    }
}

void DrawPlayerSprite(Vector2 player)
{
    const int x = static_cast<int>(std::roundf(player.x));
    const int y = static_cast<int>(std::roundf(player.y));

    DrawRectangle(x - 9, y - 23, 18, 7, { 35, 29, 35, 255 });
    DrawRectangle(x - 7, y - 17, 14, 14, { 239, 205, 168, 255 });
    DrawRectangle(x - 8, y - 4, 16, 18, { 48, 78, 104, 255 });
    DrawRectangle(x - 7, y, 14, 4, { 131, 45, 48, 255 });
    DrawRectangle(x - 11, y + 1, 4, 13, { 239, 205, 168, 255 });
    DrawRectangle(x + 7, y + 1, 4, 13, { 239, 205, 168, 255 });
    DrawRectangle(x - 8, y + 14, 6, 5, { 37, 39, 45, 255 });
    DrawRectangle(x + 2, y + 14, 6, 5, { 37, 39, 45, 255 });
}

void DrawHud(
    Font font,
    const GameState& state,
    bool chineseFontLoaded)
{
    DrawTextLocalized(font, kUiTexts[0], { 55.0f, 16.0f }, 28.0f, kPaper, chineseFontLoaded);
    DrawTextLocalized(font, kUiTexts[15], { 57.0f, 49.0f }, 14.0f, { 154, 145, 127, 255 }, chineseFontLoaded);
    DrawTextLocalized(font, kUiTexts[4], { 57.0f, 70.0f }, 15.0f, { 191, 181, 160, 255 }, chineseFontLoaded);

    const float statX = 755.0f;
    const float barWidth = 185.0f;
    const float barHeight = 8.0f;

    DrawTextLocalized(
        font,
        kUiTexts[1],
        { statX, 11.0f },
        15.0f,
        kHealth,
        chineseFontLoaded);
    DrawTextEx(
        font,
        TextFormat("%d", static_cast<int>(state.health)),
        { statX + 120.0f, 11.0f },
        15.0f,
        1.0f,
        kPaper);
    DrawRectangle(static_cast<int>(statX), 31, static_cast<int>(barWidth), static_cast<int>(barHeight), { 48, 39, 42, 255 });
    DrawRectangle(static_cast<int>(statX), 31, static_cast<int>(barWidth * state.health / 100.0f), static_cast<int>(barHeight), kHealth);

    DrawTextLocalized(
        font,
        kUiTexts[2],
        { statX, 47.0f },
        15.0f,
        kSanity,
        chineseFontLoaded);
    DrawTextEx(
        font,
        TextFormat("%d", static_cast<int>(state.sanity)),
        { statX + 120.0f, 47.0f },
        15.0f,
        1.0f,
        kPaper);
    DrawRectangle(static_cast<int>(statX), 67, static_cast<int>(barWidth), static_cast<int>(barHeight), { 38, 45, 49, 255 });
    DrawRectangle(static_cast<int>(statX), 67, static_cast<int>(barWidth * state.sanity / 100.0f), static_cast<int>(barHeight), kSanity);

    DrawTextLocalized(
        font,
        kUiTexts[3],
        { statX, 82.0f },
        15.0f,
        kTrust,
        chineseFontLoaded);
    DrawTextEx(
        font,
        TextFormat("%d", static_cast<int>(state.trust)),
        { statX + 120.0f, 82.0f },
        15.0f,
        1.0f,
        kPaper);
    DrawRectangle(static_cast<int>(statX), 101, static_cast<int>(barWidth), static_cast<int>(barHeight), { 49, 44, 34, 255 });
    DrawRectangle(static_cast<int>(statX), 101, static_cast<int>(barWidth * state.trust / 100.0f), static_cast<int>(barHeight), kTrust);

    if (state.currentChapter == ChapterId::One &&
        state.chapterOneComplete &&
        state.chapterPromptDismissed)
    {
        DrawTextLocalized(font, kUiTexts[7], { 57.0f, 88.0f }, 13.0f, kTrust, chineseFontLoaded);
    }
    else if (state.currentChapter == ChapterId::Two)
    {
        DrawTextLocalized(
            font,
            kUiTexts[kUiChapterTwoTitle],
            { 57.0f, 88.0f },
            13.0f,
            kTrust,
            chineseFontLoaded);
    }
    else if (state.currentChapter == ChapterId::Three)
    {
        DrawTextLocalized(
            font,
            kUiTexts[kUiChapterThreeTitle],
            { 57.0f, 88.0f },
            13.0f,
            kSanity,
            chineseFontLoaded);
    }
}

void DrawDialogueBox(
    Font font,
    const GameState& state,
    bool chineseFontLoaded)
{
    if (state.dialogue.lines == nullptr ||
        state.dialogue.index < 0 ||
        state.dialogue.index >= state.dialogue.count)
    {
        return;
    }

    const DialogueLine& line = state.dialogue.lines[state.dialogue.index];
    const Rectangle box = { 55.0f, 470.0f, 890.0f, 205.0f };

    DrawRectangleRec(box, { 12, 11, 14, 242 });
    DrawRectangleLinesEx(box, 4.0f, { 157, 139, 99, 255 });
    DrawRectangle(72, 486, 168, 34, { 73, 43, 39, 255 });

    DrawTextLocalized(
        font,
        line.speaker,
        { 84.0f, 491.0f },
        20.0f,
        kPaper,
        chineseFontLoaded);

    DrawWrappedText(
        font,
        Text(line.text, chineseFontLoaded),
        { 82.0f, 540.0f },
        23.0f,
        830.0f,
        31.0f,
        { 229, 224, 208, 255 });

    DrawTextLocalized(
        font,
        kUiTexts[5],
        { 760.0f, 641.0f },
        14.0f,
        { 169, 159, 136, 255 },
        chineseFontLoaded);
}

void DrawChoices(
    Font font,
    const GameState& state,
    bool chineseFontLoaded)
{
    const Rectangle box = { 55.0f, 438.0f, 890.0f, 237.0f };
    DrawRectangleRec(box, { 12, 11, 14, 244 });
    DrawRectangleLinesEx(box, 4.0f, { 157, 139, 99, 255 });

    DrawTextLocalized(
        font,
        kTeacherIntroLines[2].speaker,
        { 78.0f, 454.0f },
        19.0f,
        kPaper,
        chineseFontLoaded);
    DrawTextLocalized(
        font,
        kTeacherIntroLines[2].text,
        { 78.0f, 482.0f },
        19.0f,
        { 229, 224, 208, 255 },
        chineseFontLoaded);

    for (int i = 0; i < CountOf(kChoices); ++i)
    {
        const bool selected = (state.selectedChoice == i);
        const Rectangle optionBox = {
            78.0f,
            520.0f + static_cast<float>(i) * 42.0f,
            844.0f,
            35.0f
        };

        DrawRectangleRec(
            optionBox,
            selected ? Color{ 91, 66, 47, 255 } : Color{ 34, 31, 30, 255 });
        DrawRectangleLinesEx(
            optionBox,
            selected ? 2.0f : 1.0f,
            selected ? kTrust : Color{ 84, 76, 64, 255 });

        const std::string label =
            TextFormat("%d. %s", i + 1, Text(kChoices[i].label, chineseFontLoaded));
        DrawTextEx(
            font,
            label.c_str(),
            { optionBox.x + 12.0f, optionBox.y + 7.0f },
            17.0f,
            1.0f,
            selected ? kPaper : Color{ 194, 186, 167, 255 });
    }

    DrawTextLocalized(
        font,
        kUiTexts[6],
        { 690.0f, 647.0f },
        13.0f,
        { 169, 159, 136, 255 },
        chineseFontLoaded);
}

void DrawChapterTwoChoices(
    Font font,
    const GameState& state,
    bool chineseFontLoaded)
{
    const ChoiceOption* choices = nullptr;
    int choiceCount = 0;
    const char* prompt = "";

    if (state.chapterTwoChoiceSet == ChapterTwoChoiceSet::Exam)
    {
        choices = kChapterTwoExamChoices;
        choiceCount = CountOf(kChapterTwoExamChoices);
        prompt = Text(kChapterTwoPaperLines[0].text, chineseFontLoaded);
    }
    else if (state.chapterTwoChoiceSet == ChapterTwoChoiceSet::Cheating)
    {
        choices = kChapterTwoCheatChoices;
        choiceCount = CountOf(kChapterTwoCheatChoices);
        prompt = Text(kChapterTwoCheatIntroLines[3].text, chineseFontLoaded);
    }

    if (choices == nullptr || choiceCount <= 0)
    {
        return;
    }

    const Rectangle box = { 55.0f, 420.0f, 890.0f, 255.0f };
    DrawRectangleRec(box, { 12, 11, 14, 246 });
    DrawRectangleLinesEx(box, 4.0f, kTrust);

    DrawTextLocalized(
        font,
        kUiTexts[kUiChapterTwoTitle],
        { 78.0f, 436.0f },
        19.0f,
        kPaper,
        chineseFontLoaded);
    DrawWrappedText(
        font,
        prompt,
        { 78.0f, 463.0f },
        17.0f,
        830.0f,
        23.0f,
        { 209, 201, 181, 255 });

    for (int i = 0; i < choiceCount; ++i)
    {
        const bool selected = state.chapterTwoSelectedChoice == i;
        const Rectangle optionBox = {
            78.0f,
            517.0f + static_cast<float>(i) * 44.0f,
            844.0f,
            36.0f
        };

        DrawRectangleRec(
            optionBox,
            selected ? Color{ 83, 61, 54, 255 } : Color{ 32, 30, 30, 255 });
        DrawRectangleLinesEx(
            optionBox,
            selected ? 2.0f : 1.0f,
            selected ? kTrust : Color{ 81, 74, 66, 255 });

        const std::string label =
            TextFormat("%d. %s", i + 1, Text(choices[i].label, chineseFontLoaded));
        DrawTextEx(
            font,
            label.c_str(),
            { optionBox.x + 12.0f, optionBox.y + 8.0f },
            17.0f,
            1.0f,
            selected ? kPaper : Color{ 189, 181, 164, 255 });
    }

    DrawTextLocalized(
        font,
        kUiTexts[6],
        { 675.0f, 650.0f },
        13.0f,
        { 169, 159, 136, 255 },
        chineseFontLoaded);
}

void DrawInteractionPrompt(
    Font font,
    const Interactable* nearest,
    bool chineseFontLoaded)
{
    if (nearest == nullptr)
    {
        return;
    }

    const char* prefix = Text(kUiTexts[13], chineseFontLoaded);
    const char* name = Text(nearest->name, chineseFontLoaded);
    const std::string prompt = TextFormat("%s%s", prefix, name);
    const Vector2 size = MeasureTextEx(font, prompt.c_str(), 18.0f, 1.0f);
    const Rectangle background = {
        kScreenWidth * 0.5f - size.x * 0.5f - 16.0f,
        425.0f,
        size.x + 32.0f,
        36.0f
    };

    DrawRectangleRec(background, { 20, 18, 19, 230 });
    DrawRectangleLinesEx(background, 2.0f, kTrust);
    DrawTextEx(
        font,
        prompt.c_str(),
        { background.x + 16.0f, background.y + 8.0f },
        18.0f,
        1.0f,
        kPaper);
}

void DrawChapterComplete(
    Font font,
    GameState& state,
    bool chineseFontLoaded)
{
    DrawRectangle(0, 110, kScreenWidth, kScreenHeight - 110, Fade(BLACK, 0.78f));
    DrawRectangle(210, 238, 580, 224, { 20, 17, 17, 248 });
    DrawRectangleLinesEx({ 210.0f, 238.0f, 580.0f, 224.0f }, 4.0f, kTrust);

    DrawTextCentered(
        font,
        Text(kUiTexts[7], chineseFontLoaded),
        500.0f,
        275.0f,
        36.0f,
        kPaper);
    DrawTextCentered(
        font,
        Text(kUiTexts[8], chineseFontLoaded),
        500.0f,
        335.0f,
        25.0f,
        kTrust);
    DrawTextCentered(
        font,
        Text(kUiTexts[9], chineseFontLoaded),
        500.0f,
        405.0f,
        16.0f,
        { 185, 175, 155, 255 });

    if (IsKeyPressed(KEY_ENTER))
    {
        state.chapterPromptDismissed = true;
        EnterChapterTwo(state);
    }
}

void DrawChapterTwoComplete(
    Font font,
    GameState& state,
    bool chineseFontLoaded)
{
    DrawRectangle(0, 110, kScreenWidth, kScreenHeight - 110, Fade(BLACK, 0.82f));
    DrawRectangle(180, 225, 640, 250, { 20, 17, 17, 248 });
    DrawRectangleLinesEx({ 180.0f, 225.0f, 640.0f, 250.0f }, 4.0f, kTrust);

    DrawTextCentered(
        font,
        Text(kUiTexts[kUiChapterTwoComplete], chineseFontLoaded),
        500.0f,
        258.0f,
        38.0f,
        kPaper);
    DrawTextCentered(
        font,
        Text(kUiTexts[kUiChapterThreeComing], chineseFontLoaded),
        500.0f,
        322.0f,
        23.0f,
        kTrust);
    DrawTextCentered(
        font,
        Text(kUiTexts[kUiChapterThreePrompt], chineseFontLoaded),
        500.0f,
        414.0f,
        16.0f,
        { 185, 175, 155, 255 });

    if (IsKeyPressed(KEY_ENTER))
    {
        state.currentChapter = ChapterId::Three;
        state.currentScene = SceneId::ChapterThreePlaceholder;
        state.chapterPromptDismissed = true;
    }
}

void DrawChapterThreePlaceholder(
    Font font,
    bool chineseFontLoaded)
{
    DrawRectangle(0, 110, kScreenWidth, kScreenHeight - 110, { 13, 12, 15, 248 });
    DrawRectangle(170, 235, 660, 220, { 22, 18, 22, 250 });
    DrawRectangleLinesEx({ 170.0f, 235.0f, 660.0f, 220.0f }, 4.0f, kSanity);

    DrawTextCentered(
        font,
        Text(kUiTexts[kUiChapterThreeTitle], chineseFontLoaded),
        500.0f,
        275.0f,
        36.0f,
        kPaper);
    DrawTextCentered(
        font,
        Text(kUiTexts[kUiChapterThreePlaceholder], chineseFontLoaded),
        500.0f,
        350.0f,
        21.0f,
        { 185, 175, 155, 255 });
}

void DrawGameOver(Font font, bool chineseFontLoaded)
{
    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, Fade(BLACK, 0.78f));
    DrawTextCentered(font, "GAME OVER", 500.0f, 285.0f, 48.0f, kBlood);
    DrawTextCentered(
        font,
        Text(kUiTexts[16], chineseFontLoaded),
        500.0f,
        350.0f,
        20.0f,
        kPaper);
}

Interactable MakeInteractable(
    InteractableKind kind,
    LocalizedText name,
    Vector2 point,
    float radius)
{
    return { kind, name, point, radius, false };
}

Interactable* FindNearestInteractable(GameState& state, Interactable* interactables, int count)
{
    Interactable* nearest = nullptr;
    float nearestDistance = 1000000.0f;

    for (int i = 0; i < count; ++i)
    {
        Interactable& candidate = interactables[i];

        if (candidate.investigated && candidate.kind != InteractableKind::Teacher)
        {
            continue;
        }

        if (candidate.kind == InteractableKind::Teacher && state.teacherChoiceMade)
        {
            // Teacher B remains available for one post-choice warning.
        }

        const float distance = Vector2Distance(state.player, candidate.point);
        if (distance <= candidate.radius && distance < nearestDistance)
        {
            nearest = &candidate;
            nearestDistance = distance;
        }
    }

    return nearest;
}

void OpenInteractable(GameState& state, Interactable& interactable)
{
    switch (interactable.kind)
    {
        case InteractableKind::Blackboard:
            if (!interactable.investigated)
            {
                interactable.investigated = true;
                state.investigatedCount++;
                ApplyStatDelta(state, 0, -3, 0);
                StartDialogue(
                    state,
                    kBlackboardLines,
                    CountOf(kBlackboardLines),
                    DialogueOutcome::ResumeExploring);
            }
            break;

        case InteractableKind::Desk:
            if (!interactable.investigated)
            {
                interactable.investigated = true;
                state.investigatedCount++;
                ApplyStatDelta(state, 0, -5, 0);
                StartDialogue(
                    state,
                    kDeskLines,
                    CountOf(kDeskLines),
                    DialogueOutcome::ResumeExploring);
            }
            break;

        case InteractableKind::Door:
            if (!interactable.investigated)
            {
                interactable.investigated = true;
                state.investigatedCount++;
                StartDialogue(
                    state,
                    kDoorLines,
                    CountOf(kDoorLines),
                    DialogueOutcome::ResumeExploring);
            }
            break;

        case InteractableKind::Teacher:
            if (!state.teacherChoiceMade)
            {
                StartDialogue(
                    state,
                    kTeacherIntroLines,
                    CountOf(kTeacherIntroLines),
                    DialogueOutcome::BeginChoices);
            }
            else
            {
                StartDialogue(
                    state,
                    kTeacherAfterChoiceLines,
                    CountOf(kTeacherAfterChoiceLines),
                    DialogueOutcome::ResumeExploring);
            }
            break;
    }
}

void OpenChapterTwoInteractable(GameState& state, Interactable& interactable)
{
    switch (interactable.kind)
    {
        case InteractableKind::ExamPaperTwo:
            if (!interactable.investigated)
            {
                interactable.investigated = true;
                state.chapterTwoPaperFound = true;
                StartDialogue(
                    state,
                    kChapterTwoPaperLines,
                    CountOf(kChapterTwoPaperLines),
                    DialogueOutcome::BeginChapterTwoExamChoices);
            }
            break;

        case InteractableKind::BlackboardTwo:
            if (!interactable.investigated)
            {
                interactable.investigated = true;
                state.chapterTwoBlackboardFound = true;
                ApplyStatDelta(state, 0, -3, 0);
                StartDialogue(
                    state,
                    kChapterTwoBlackboardLines,
                    CountOf(kChapterTwoBlackboardLines),
                    DialogueOutcome::ResumeExploring);
            }
            break;

        case InteractableKind::StudentTwo:
            if (!state.chapterTwoExamChoiceMade ||
                !state.chapterTwoPaperFound ||
                !state.chapterTwoBlackboardFound)
            {
                interactable.investigated = false;
                StartDialogue(
                    state,
                    kChapterTwoStudentPreludeLines,
                    CountOf(kChapterTwoStudentPreludeLines),
                    DialogueOutcome::ResumeExploring);
            }
            else if (!state.chapterTwoStudentSpoken)
            {
                interactable.investigated = true;
                state.chapterTwoStudentSpoken = true;
                StartDialogue(
                    state,
                    kChapterTwoCheatIntroLines,
                    CountOf(kChapterTwoCheatIntroLines),
                    DialogueOutcome::BeginChapterTwoCheatChoices);
            }
            break;

        case InteractableKind::EscapeDoorTwo:
            StartDialogue(
                state,
                kChapterTwoEscapeSuccessLines,
                CountOf(kChapterTwoEscapeSuccessLines),
                DialogueOutcome::FinishChapterTwo);
            break;

        default:
            break;
    }
}

void ChooseOption(GameState& state)
{
    const ChoiceOption& choice = kChoices[state.selectedChoice];
    ApplyStatDelta(state, choice.healthDelta, choice.sanityDelta, choice.trustDelta);
    state.teacherChoiceMade = true;

    StartDialogue(
        state,
        choice.response,
        choice.responseCount,
        DialogueOutcome::FinishChapter);
}

void ChooseChapterTwoOption(GameState& state)
{
    if (state.chapterTwoChoiceSet == ChapterTwoChoiceSet::Exam)
    {
        const ChoiceOption& choice = kChapterTwoExamChoices[state.chapterTwoSelectedChoice];
        ApplyStatDelta(state, choice.healthDelta, choice.sanityDelta, choice.trustDelta);
        state.chapterTwoExamChoiceMade = true;
        state.chapterTwoChoiceSet = ChapterTwoChoiceSet::None;
        StartDialogue(
            state,
            choice.response,
            choice.responseCount,
            DialogueOutcome::ResumeExploring);
    }
    else if (state.chapterTwoChoiceSet == ChapterTwoChoiceSet::Cheating)
    {
        const ChoiceOption& choice = kChapterTwoCheatChoices[state.chapterTwoSelectedChoice];
        ApplyStatDelta(state, choice.healthDelta, choice.sanityDelta, choice.trustDelta);
        state.chapterTwoCheatChoiceMade = true;
        state.chapterTwoChoiceSet = ChapterTwoChoiceSet::None;
        StartDialogue(
            state,
            choice.response,
            choice.responseCount,
            DialogueOutcome::BeginDemonTransformation);
    }
}
}

int main()
{
    InitWindow(kScreenWidth, kScreenHeight, "异界逃生：考试倒计时");
    SetTargetFPS(60);

    bool chineseFontLoaded = false;
    Font font = LoadGameFont(chineseFontLoaded);

    GameState state;
    state.dialogue.lines = kOpeningLines;
    state.dialogue.count = CountOf(kOpeningLines);
    state.dialogue.index = 0;
    state.dialogue.outcome = DialogueOutcome::ResumeExploring;

    Interactable chapterOneInteractables[] = {
        MakeInteractable(
            InteractableKind::Blackboard,
            { "黑板", "Blackboard" },
            { 365.0f, 245.0f },
            95.0f),
        MakeInteractable(
            InteractableKind::Desk,
            { "邻座恶魔学生", "Neighboring demon student" },
            { 520.0f, 484.0f },
            75.0f),
        MakeInteractable(
            InteractableKind::Door,
            { "神秘钥匙", "Mysterious key" },
            { 100.0f, 630.0f },
            65.0f),
        MakeInteractable(
            InteractableKind::Teacher,
            { "老师讲台", "Teacher's desk" },
            { 755.0f, 340.0f },
            78.0f)
    };

    Interactable chapterTwoExamInteractables[] = {
        MakeInteractable(
            InteractableKind::ExamPaperTwo,
            { Text(kUiTexts[kUiExamPaperTwo], true), Text(kUiTexts[kUiExamPaperTwo], false) },
            { 420.0f, 410.0f },
            78.0f),
        MakeInteractable(
            InteractableKind::BlackboardTwo,
            { Text(kUiTexts[kUiBlackboardTwo], true), Text(kUiTexts[kUiBlackboardTwo], false) },
            { 300.0f, 250.0f },
            95.0f),
        MakeInteractable(
            InteractableKind::StudentTwo,
            { Text(kUiTexts[kUiStudentTwo], true), Text(kUiTexts[kUiStudentTwo], false) },
            { 790.0f, 420.0f },
            75.0f)
    };

    Interactable chapterTwoEscapeInteractables[] = {
        MakeInteractable(
            InteractableKind::EscapeDoorTwo,
            { Text(kUiTexts[kUiEscapeDoorTwo], true), Text(kUiTexts[kUiEscapeDoorTwo], false) },
            { 880.0f, 270.0f },
            78.0f)
    };

    while (!WindowShouldClose())
    {
        const float deltaTime = GetFrameTime();
        const bool advancePressed =
            IsKeyPressed(KEY_E) ||
            IsKeyPressed(KEY_ENTER) ||
            IsKeyPressed(KEY_SPACE);

        Interactable* activeInteractables = nullptr;
        int activeInteractableCount = 0;

        if (state.currentScene == SceneId::ClassroomOne)
        {
            activeInteractables = chapterOneInteractables;
            activeInteractableCount = CountOf(chapterOneInteractables);
        }
        else if (state.currentScene == SceneId::ExamRoomTwo)
        {
            activeInteractables = chapterTwoExamInteractables;
            activeInteractableCount = CountOf(chapterTwoExamInteractables);
        }
        else if (state.currentScene == SceneId::EscapeHallTwo)
        {
            activeInteractables = chapterTwoEscapeInteractables;
            activeInteractableCount = CountOf(chapterTwoEscapeInteractables);
        }

        switch (state.phase)
        {
            case Phase::Opening:
            case Phase::Dialogue:
                if (advancePressed)
                {
                    AdvanceDialogue(state);
                }
                break;

            case Phase::Exploring:
            {
                MovePlayer(state);
                UpdateDemonChase(state, deltaTime);

                if (state.currentScene == SceneId::EscapeHallTwo &&
                    state.chapterTwoContractTriggered &&
                    !state.chapterTwoComplete &&
                    Vector2Distance(state.player, { 885.0f, 270.0f }) <= 62.0f)
                {
                    StartDialogue(
                        state,
                        kChapterTwoEscapeSuccessLines,
                        CountOf(kChapterTwoEscapeSuccessLines),
                        DialogueOutcome::FinishChapterTwo);
                    break;
                }

                Interactable* nearest =
                    FindNearestInteractable(state, activeInteractables, activeInteractableCount);

                if (nearest != nullptr && IsKeyPressed(KEY_E))
                {
                    if (state.currentChapter == ChapterId::One)
                    {
                        OpenInteractable(state, *nearest);
                    }
                    else if (state.currentChapter == ChapterId::Two)
                    {
                        OpenChapterTwoInteractable(state, *nearest);
                    }
                }
                break;
            }

            case Phase::Choices:
                if (state.currentChapter == ChapterId::One)
                {
                    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
                    {
                        state.selectedChoice =
                            (state.selectedChoice + CountOf(kChoices) - 1) % CountOf(kChoices);
                    }
                    else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
                    {
                        state.selectedChoice =
                            (state.selectedChoice + 1) % CountOf(kChoices);
                    }
                    else if (IsKeyPressed(KEY_ONE))
                    {
                        state.selectedChoice = 0;
                    }
                    else if (IsKeyPressed(KEY_TWO))
                    {
                        state.selectedChoice = 1;
                    }
                    else if (IsKeyPressed(KEY_THREE))
                    {
                        state.selectedChoice = 2;
                    }

                    if (advancePressed)
                    {
                        ChooseOption(state);
                    }
                }
                else if (state.currentChapter == ChapterId::Two)
                {
                    int* selected = &state.chapterTwoSelectedChoice;
                    const int choiceCount =
                        (state.chapterTwoChoiceSet == ChapterTwoChoiceSet::Exam)
                            ? CountOf(kChapterTwoExamChoices)
                            : CountOf(kChapterTwoCheatChoices);

                    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
                    {
                        *selected = (*selected + choiceCount - 1) % choiceCount;
                    }
                    else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
                    {
                        *selected = (*selected + 1) % choiceCount;
                    }
                    else if (IsKeyPressed(KEY_ONE))
                    {
                        *selected = 0;
                    }
                    else if (IsKeyPressed(KEY_TWO))
                    {
                        *selected = 1;
                    }
                    else if (IsKeyPressed(KEY_THREE))
                    {
                        *selected = 2;
                    }

                    if (advancePressed)
                    {
                        ChooseChapterTwoOption(state);
                    }
                }
                break;

            case Phase::ChapterComplete:
            case Phase::GameOver:
                break;
        }

        if (state.health <= 0.0f && state.phase != Phase::GameOver)
        {
            state.phase = Phase::GameOver;
        }

        BeginDrawing();
        ClearBackground(kInk);

        if (state.currentScene == SceneId::ClassroomOne)
        {
            DrawClassroom(font, chineseFontLoaded, state);
        }
        else if (state.currentScene == SceneId::ExamRoomTwo)
        {
            DrawExamRoomTwo(font, chineseFontLoaded, state);
        }
        else if (state.currentScene == SceneId::EscapeHallTwo)
        {
            DrawEscapeHallTwo(font, chineseFontLoaded, state);
        }
        else
        {
            DrawRectangle(0, 108, kScreenWidth, kScreenHeight - 108, { 18, 17, 21, 255 });
        }

        DrawPlayerSprite(state.player);
        DrawHud(font, state, chineseFontLoaded);

        if (activeInteractables != nullptr &&
            activeInteractableCount > 0 &&
            (state.phase == Phase::Exploring || state.phase == Phase::Dialogue))
        {
            Interactable* nearest =
                FindNearestInteractable(state, activeInteractables, activeInteractableCount);
            if (state.phase == Phase::Exploring)
            {
                DrawInteractionPrompt(font, nearest, chineseFontLoaded);
            }
        }

        if (state.phase == Phase::Opening || state.phase == Phase::Dialogue)
        {
            DrawDialogueBox(font, state, chineseFontLoaded);
        }
        else if (state.phase == Phase::Choices)
        {
            if (state.currentChapter == ChapterId::One)
            {
                DrawChoices(font, state, chineseFontLoaded);
            }
            else if (state.currentChapter == ChapterId::Two)
            {
                DrawChapterTwoChoices(font, state, chineseFontLoaded);
            }
        }
        else if (state.phase == Phase::ChapterComplete)
        {
            if (state.currentChapter == ChapterId::One)
            {
                DrawChapterComplete(font, state, chineseFontLoaded);
            }
            else if (state.currentChapter == ChapterId::Two)
            {
                DrawChapterTwoComplete(font, state, chineseFontLoaded);
            }
            else
            {
                DrawChapterThreePlaceholder(font, chineseFontLoaded);
            }
        }

        if (!chineseFontLoaded)
        {
            DrawRectangle(58, 108, 884, 25, { 87, 41, 37, 235 });
            DrawTextCentered(
                font,
                Text(kUiTexts[14], false),
                500.0f,
                113.0f,
                14.0f,
                kPaper);
        }

        if (state.phase == Phase::GameOver)
        {
            DrawGameOver(font, chineseFontLoaded);
        }

        EndDrawing();
    }

    if (chineseFontLoaded)
    {
        UnloadFont(font);
    }

    CloseWindow();
    return 0;
}
