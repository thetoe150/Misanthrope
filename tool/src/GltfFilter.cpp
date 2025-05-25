#include "GltfFilter.h"

void testRapidJson() {
	rapidjson::Document doc;
	doc.SetObject();
	rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

	rapidjson::Value object1{rapidjson::kObjectType};
	object1.AddMember("key1", "value1", allocator);
	object1.AddMember("key2", "value2", allocator);
	doc.AddMember("object1", object1, allocator);
	doc.AddMember("key3", "value3", allocator);

    // bool removed = doc.RemoveMember("object1");
	// if (removed)
	// 	printf("object1 is removed\n");
    // doc.AddMember("object1", object1, allocator);
    // // Remove object by iterator using EraseMember
    // auto it = doc.FindMember("object1");
    // if (it != doc.MemberEnd()) {
    //     doc.EraseMember(it);
	// }

	rapidjson::StringBuffer buffer; 
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	printf("output string: %s\n", buffer.GetString());
}
