#include "allocator.hpp"

void printMemoryBudget(){
	VmaBudget* budgets = new VmaBudget[m_allocator->GetMemoryHeapCount()];
	vmaGetHeapBudgets(m_allocator, budgets);
	
	std::cout << "Number of Heaps is: " << m_allocator->GetMemoryHeapCount() << "\n" <<
				"Number of Types is: " <<m_allocator->GetMemoryTypeCount() << "\n";

	for(unsigned int i = 0; i < m_allocator->GetMemoryHeapCount(); i++)
	{
		std::cout <<
		"Heap index: " << i << "\n" <<
		"Number of `VkDeviceMemory` objects						: " << budgets[i].statistics.blockCount << "\n" <<
		"Number of #VmaAllocation objects allocated				: " << budgets[i].statistics.allocationCount << "\n" <<
		"Number of bytes allocated in `VkDeviceMemory` blocks	: " << budgets[i].statistics.blockBytes << "\n" <<
		"Number of bytes occupied by all #VmaAllocation objects	: " << budgets[i].statistics.allocationBytes << "\n" <<
		"Estimated current memory usage of the program, in bytes: " << budgets[i].usage << "\n" <<
		"Estimated amount of memory available to the program, in bytes: " << budgets[i].budget << "\n"
		"\n";
	}

	delete[] budgets;
}

void printMemoryStatistics(){
	enum class StatType{
		TYPE,
		HEAP,
		TOTAL,
	};

	VkPhysicalDeviceMemoryProperties memProperties{};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	auto l_printStat = [&memProperties](VmaDetailedStatistics* stats, unsigned int size, StatType type) -> void{
		for(unsigned int i = 0; i < size; i++)
		{
			if(type == StatType::TOTAL){
			}
			else if(type == StatType::HEAP){
				std::cout << "Heap index "<< i << ": " << vk::to_string((vk::MemoryHeapFlags)memProperties.memoryHeaps[i].flags) 
				<< ", with size: " << memProperties.memoryHeaps[i].size << "\n";
			}
			else if (type == StatType::TYPE){
				std::cout << "Type index "<< i << ": " << vk::to_string((vk::MemoryPropertyFlags)memProperties.memoryTypes[i].propertyFlags)
				<< ", belong to heap index: " << memProperties.memoryTypes[i].heapIndex  << "\n";
			}

			VmaStatistics basicStat = stats[i].statistics;
			std::cout << "Current usage: \n" <<
			"Number of `VkDeviceMemory` objects - Vulkan memory blocks allocated: " << basicStat.blockCount << "\n" <<
			"Number of #VmaAllocation objects allocated: " << basicStat.allocationCount << "\n" <<
			"Number of bytes allocated in `VkDeviceMemory` blocks: " << basicStat.blockBytes << "\n" <<
			"Total number of bytes occupied by all #VmaAllocation objects: " << basicStat.allocationBytes << "\n" <<
			"Number of free ranges of memory between allocations: " << stats[i].unusedRangeCount << "\n" <<
			"Smallest allocation size: " << stats[i].allocationSizeMin << "\n" <<
			"Largest allocation size: " << stats[i].allocationSizeMax << "\n" <<
			"Smallest empty range size: " << stats[i].unusedRangeSizeMin << "\n" <<
			"Largest empty range size: " << stats[i].unusedRangeSizeMax << "\n" <<
			"\n";
		}
	};

	VmaTotalStatistics stats{};
	vmaCalculateStatistics(m_allocator, &stats);

	std::cout << "\n ####### Total statistics: #######\n";
	l_printStat(&stats.total, 1, StatType::TOTAL);

	std::cout << "\n ####### Heap statistics: #######\n";
	unsigned int heapCount = m_allocator->GetMemoryHeapCount();
	l_printStat(stats.memoryHeap, heapCount, StatType::HEAP);

	std::cout << "\n ####### Type statistics: #######\n";
	unsigned int typeCount = m_allocator->GetMemoryTypeCount();
	l_printStat(stats.memoryType, typeCount, StatType::TYPE);

	// l_printStat(stats.total, typeCount);
}
