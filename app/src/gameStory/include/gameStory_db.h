#pragma once

#include <string>
#include <vector>

struct DialogueChoice {
    int         choiceIdx = 0;
    std::string label;
    int         nextNodeId = 0;
};

struct DialogueNode {
    int                     nodeId = 0;
    std::string             speaker;
    std::string             text;
    std::string             imageUrl;
    std::string             bgmUrl;
    std::string             sfxUrl;
    int                     nextNodeId = 0;
    bool                    hasChoices = false;
};

bool storyDbOpen();
void storyDbClose();
std::vector<DialogueNode> storyDbLoadDialogue(int idStory, int idChapter);
std::vector<DialogueChoice> storyDbLoadChoices(int idStory, int idChapter,
                                                int nodeId);