#include <Strata/ui.hpp>

using namespace strata::ui;


enum class Priority{
    Lowest,
    Low,
    Moderate,
    High,
    Critical

};

struct stickyNote{

    std::string title = "";
    std::string content="";
    Priority priority = Priority::Lowest;

};

std::string priorityToText(Priority pri){
    switch (pri)
    {
    case Priority::Lowest:
        return "Lowest";
    case Priority::Low:
        return "Low";
    case Priority::Moderate:
        return "Moderate";
    case Priority::High:
        return "High";
    case Priority::Critical:
        return "Critical";
    default:
        return "Unknown";
    }
}



Node Card(const stickyNote& note){
    return Block(note.title).inner(
        Col({
            Block().inner(
                Label(note.content).wrap(true)
            ).size(percentage(70)).cross(percentage(50)),

            Label("Priority:"+priorityToText(note.priority))
        }).gap(1)
    ).size(fixed(10)).cross(fixed(40));
}


auto notes = make_state(std::vector<stickyNote>{
    {"deniz","flajfakfafkaefka",Priority::Low},
    {"deniz","flajfakfafkaefka",Priority::Low},
    {"deniz","flajfakfafkaefka",Priority::Low},
    {"deniz","flajfakfafkaefka",Priority::Low},
    {"deniz","flajfakfafkaefka",Priority::Low},

});



int main(){
    App app;

    populate(app,{
        Block("StickyNotes").inner(
            ScrollView({
                ForEach(notes,[](const stickyNote& item,int index)->std::optional<Node>{
                    return Card(item);
                })
            })
        )
    }
    );



    app.run();
    return 0;
}
