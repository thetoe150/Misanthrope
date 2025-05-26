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

void filterGltf(uint8_t* jsonBlob, uint32_t jsonLength, uint8_t* gltfBlob, uint32_t gltfSize) {

	rapidjson::MemoryStream ms((char*)jsonBlob, jsonLength);
	rapidjson::Document doc;
    rapidjson::ParseResult result = doc.ParseStream(ms);
    if (!result) {
        printf("JSON parse error: %s (at offset %llu)\n", rapidjson::GetParseError_En(result.Code()), result.Offset()); 
    }

	rapidjson::StringBuffer buffer; 
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    writer.SetIndent(' ', 4); // 4 spaces
    writer.SetFormatOptions( rapidjson::kFormatDefault);
	doc.Accept(writer);
	printf("output string: %s\n", buffer.GetString());
}
