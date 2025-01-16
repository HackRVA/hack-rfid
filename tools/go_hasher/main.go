package main

import (
	"fmt"
	"sort"
)

// aclHash needs to be a simple hashing algorithm that we can run on the server (which is golang)
// and on the mcu (embeded C)
func aclHash(users []string) uint32 {
	sort.Strings(users)

	var hash uint32 = 5381

	for _, user := range users {
		for _, ch := range user {
			fmt.Printf("[HASH] Adding char '%c' (0x%02X) to hash.\n", ch, uint8(ch))
			hash = ((hash << 5) + hash) + uint32(ch)
		}
		println("[HASH] Adding separator 0xFE to hash.")
		hash = ((hash << 5) + hash) + 0xFE
	}

	fmt.Printf("[HASH] Final hash value: 0x%08X\n", hash)
	return hash
}

func main() {
	users := []string{
		"alice",
		"bob",
		"charlie",
		"allen",
		// "alf",
		"chris",
	}
	hash := aclHash(users)
	fmt.Printf("ACL Hash: %d\n", hash)
}
