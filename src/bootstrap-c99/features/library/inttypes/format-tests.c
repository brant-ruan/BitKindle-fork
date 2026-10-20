static int format_tests(void) {
    char buffer[128];
    int8_t v_d8 = 42;
    if (sprintf(buffer, "%" PRId8, v_d8) != 2 || strcmp(buffer, "42"))
        return 1;
    int8_t v_i8 = 42;
    if (sprintf(buffer, "%" PRIi8, v_i8) != 2 || strcmp(buffer, "42"))
        return 1;
    uint8_t v_o8 = 42;
    if (sprintf(buffer, "%" PRIo8, v_o8) != 2 || strcmp(buffer, "52"))
        return 1;
    uint8_t v_u8 = 42;
    if (sprintf(buffer, "%" PRIu8, v_u8) != 2 || strcmp(buffer, "42"))
        return 1;
    uint8_t v_x8 = 42;
    if (sprintf(buffer, "%" PRIx8, v_x8) != 2 || strcmp(buffer, "2a"))
        return 1;
    uint8_t v_X8 = 42;
    if (sprintf(buffer, "%" PRIX8, v_X8) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            int8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNd8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            int8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNi8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNo8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNu8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNx8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    int16_t v_d16 = 42;
    if (sprintf(buffer, "%" PRId16, v_d16) != 2 || strcmp(buffer, "42"))
        return 1;
    int16_t v_i16 = 42;
    if (sprintf(buffer, "%" PRIi16, v_i16) != 2 || strcmp(buffer, "42"))
        return 1;
    uint16_t v_o16 = 42;
    if (sprintf(buffer, "%" PRIo16, v_o16) != 2 || strcmp(buffer, "52"))
        return 1;
    uint16_t v_u16 = 42;
    if (sprintf(buffer, "%" PRIu16, v_u16) != 2 || strcmp(buffer, "42"))
        return 1;
    uint16_t v_x16 = 42;
    if (sprintf(buffer, "%" PRIx16, v_x16) != 2 || strcmp(buffer, "2a"))
        return 1;
    uint16_t v_X16 = 42;
    if (sprintf(buffer, "%" PRIX16, v_X16) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            int16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNd16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            int16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNi16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNo16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNu16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNx16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    int32_t v_d32 = 42;
    if (sprintf(buffer, "%" PRId32, v_d32) != 2 || strcmp(buffer, "42"))
        return 1;
    int32_t v_i32 = 42;
    if (sprintf(buffer, "%" PRIi32, v_i32) != 2 || strcmp(buffer, "42"))
        return 1;
    uint32_t v_o32 = 42;
    if (sprintf(buffer, "%" PRIo32, v_o32) != 2 || strcmp(buffer, "52"))
        return 1;
    uint32_t v_u32 = 42;
    if (sprintf(buffer, "%" PRIu32, v_u32) != 2 || strcmp(buffer, "42"))
        return 1;
    uint32_t v_x32 = 42;
    if (sprintf(buffer, "%" PRIx32, v_x32) != 2 || strcmp(buffer, "2a"))
        return 1;
    uint32_t v_X32 = 42;
    if (sprintf(buffer, "%" PRIX32, v_X32) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            int32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNd32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            int32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNi32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNo32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNu32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNx32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    int64_t v_d64 = 42;
    if (sprintf(buffer, "%" PRId64, v_d64) != 2 || strcmp(buffer, "42"))
        return 1;
    int64_t v_i64 = 42;
    if (sprintf(buffer, "%" PRIi64, v_i64) != 2 || strcmp(buffer, "42"))
        return 1;
    uint64_t v_o64 = 42;
    if (sprintf(buffer, "%" PRIo64, v_o64) != 2 || strcmp(buffer, "52"))
        return 1;
    uint64_t v_u64 = 42;
    if (sprintf(buffer, "%" PRIu64, v_u64) != 2 || strcmp(buffer, "42"))
        return 1;
    uint64_t v_x64 = 42;
    if (sprintf(buffer, "%" PRIx64, v_x64) != 2 || strcmp(buffer, "2a"))
        return 1;
    uint64_t v_X64 = 42;
    if (sprintf(buffer, "%" PRIX64, v_X64) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            int64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNd64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            int64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNi64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNo64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNu64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNx64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    int_least8_t v_dLEAST8 = 42;
    if (sprintf(buffer, "%" PRIdLEAST8, v_dLEAST8) != 2 || strcmp(buffer, "42"))
        return 1;
    int_least8_t v_iLEAST8 = 42;
    if (sprintf(buffer, "%" PRIiLEAST8, v_iLEAST8) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_least8_t v_oLEAST8 = 42;
    if (sprintf(buffer, "%" PRIoLEAST8, v_oLEAST8) != 2 || strcmp(buffer, "52"))
        return 1;
    uint_least8_t v_uLEAST8 = 42;
    if (sprintf(buffer, "%" PRIuLEAST8, v_uLEAST8) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_least8_t v_xLEAST8 = 42;
    if (sprintf(buffer, "%" PRIxLEAST8, v_xLEAST8) != 2 || strcmp(buffer, "2a"))
        return 1;
    uint_least8_t v_XLEAST8 = 42;
    if (sprintf(buffer, "%" PRIXLEAST8, v_XLEAST8) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            int_least8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNdLEAST8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            int_least8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNiLEAST8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_least8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNoLEAST8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_least8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNuLEAST8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_least8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNxLEAST8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    int_least16_t v_dLEAST16 = 42;
    if (sprintf(buffer, "%" PRIdLEAST16, v_dLEAST16) != 2 || strcmp(buffer, "42"))
        return 1;
    int_least16_t v_iLEAST16 = 42;
    if (sprintf(buffer, "%" PRIiLEAST16, v_iLEAST16) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_least16_t v_oLEAST16 = 42;
    if (sprintf(buffer, "%" PRIoLEAST16, v_oLEAST16) != 2 || strcmp(buffer, "52"))
        return 1;
    uint_least16_t v_uLEAST16 = 42;
    if (sprintf(buffer, "%" PRIuLEAST16, v_uLEAST16) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_least16_t v_xLEAST16 = 42;
    if (sprintf(buffer, "%" PRIxLEAST16, v_xLEAST16) != 2 || strcmp(buffer, "2a"))
        return 1;
    uint_least16_t v_XLEAST16 = 42;
    if (sprintf(buffer, "%" PRIXLEAST16, v_XLEAST16) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            int_least16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNdLEAST16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            int_least16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNiLEAST16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_least16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNoLEAST16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_least16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNuLEAST16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_least16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNxLEAST16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    int_least32_t v_dLEAST32 = 42;
    if (sprintf(buffer, "%" PRIdLEAST32, v_dLEAST32) != 2 || strcmp(buffer, "42"))
        return 1;
    int_least32_t v_iLEAST32 = 42;
    if (sprintf(buffer, "%" PRIiLEAST32, v_iLEAST32) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_least32_t v_oLEAST32 = 42;
    if (sprintf(buffer, "%" PRIoLEAST32, v_oLEAST32) != 2 || strcmp(buffer, "52"))
        return 1;
    uint_least32_t v_uLEAST32 = 42;
    if (sprintf(buffer, "%" PRIuLEAST32, v_uLEAST32) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_least32_t v_xLEAST32 = 42;
    if (sprintf(buffer, "%" PRIxLEAST32, v_xLEAST32) != 2 || strcmp(buffer, "2a"))
        return 1;
    uint_least32_t v_XLEAST32 = 42;
    if (sprintf(buffer, "%" PRIXLEAST32, v_XLEAST32) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            int_least32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNdLEAST32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            int_least32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNiLEAST32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_least32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNoLEAST32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_least32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNuLEAST32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_least32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNxLEAST32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    int_least64_t v_dLEAST64 = 42;
    if (sprintf(buffer, "%" PRIdLEAST64, v_dLEAST64) != 2 || strcmp(buffer, "42"))
        return 1;
    int_least64_t v_iLEAST64 = 42;
    if (sprintf(buffer, "%" PRIiLEAST64, v_iLEAST64) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_least64_t v_oLEAST64 = 42;
    if (sprintf(buffer, "%" PRIoLEAST64, v_oLEAST64) != 2 || strcmp(buffer, "52"))
        return 1;
    uint_least64_t v_uLEAST64 = 42;
    if (sprintf(buffer, "%" PRIuLEAST64, v_uLEAST64) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_least64_t v_xLEAST64 = 42;
    if (sprintf(buffer, "%" PRIxLEAST64, v_xLEAST64) != 2 || strcmp(buffer, "2a"))
        return 1;
    uint_least64_t v_XLEAST64 = 42;
    if (sprintf(buffer, "%" PRIXLEAST64, v_XLEAST64) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            int_least64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNdLEAST64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            int_least64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNiLEAST64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_least64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNoLEAST64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_least64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNuLEAST64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_least64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNxLEAST64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    int_fast8_t v_dFAST8 = 42;
    if (sprintf(buffer, "%" PRIdFAST8, v_dFAST8) != 2 || strcmp(buffer, "42"))
        return 1;
    int_fast8_t v_iFAST8 = 42;
    if (sprintf(buffer, "%" PRIiFAST8, v_iFAST8) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_fast8_t v_oFAST8 = 42;
    if (sprintf(buffer, "%" PRIoFAST8, v_oFAST8) != 2 || strcmp(buffer, "52"))
        return 1;
    uint_fast8_t v_uFAST8 = 42;
    if (sprintf(buffer, "%" PRIuFAST8, v_uFAST8) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_fast8_t v_xFAST8 = 42;
    if (sprintf(buffer, "%" PRIxFAST8, v_xFAST8) != 2 || strcmp(buffer, "2a"))
        return 1;
    uint_fast8_t v_XFAST8 = 42;
    if (sprintf(buffer, "%" PRIXFAST8, v_XFAST8) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            int_fast8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNdFAST8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            int_fast8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNiFAST8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_fast8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNoFAST8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_fast8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNuFAST8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_fast8_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNxFAST8, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    int_fast16_t v_dFAST16 = 42;
    if (sprintf(buffer, "%" PRIdFAST16, v_dFAST16) != 2 || strcmp(buffer, "42"))
        return 1;
    int_fast16_t v_iFAST16 = 42;
    if (sprintf(buffer, "%" PRIiFAST16, v_iFAST16) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_fast16_t v_oFAST16 = 42;
    if (sprintf(buffer, "%" PRIoFAST16, v_oFAST16) != 2 || strcmp(buffer, "52"))
        return 1;
    uint_fast16_t v_uFAST16 = 42;
    if (sprintf(buffer, "%" PRIuFAST16, v_uFAST16) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_fast16_t v_xFAST16 = 42;
    if (sprintf(buffer, "%" PRIxFAST16, v_xFAST16) != 2 || strcmp(buffer, "2a"))
        return 1;
    uint_fast16_t v_XFAST16 = 42;
    if (sprintf(buffer, "%" PRIXFAST16, v_XFAST16) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            int_fast16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNdFAST16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            int_fast16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNiFAST16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_fast16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNoFAST16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_fast16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNuFAST16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_fast16_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNxFAST16, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    int_fast32_t v_dFAST32 = 42;
    if (sprintf(buffer, "%" PRIdFAST32, v_dFAST32) != 2 || strcmp(buffer, "42"))
        return 1;
    int_fast32_t v_iFAST32 = 42;
    if (sprintf(buffer, "%" PRIiFAST32, v_iFAST32) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_fast32_t v_oFAST32 = 42;
    if (sprintf(buffer, "%" PRIoFAST32, v_oFAST32) != 2 || strcmp(buffer, "52"))
        return 1;
    uint_fast32_t v_uFAST32 = 42;
    if (sprintf(buffer, "%" PRIuFAST32, v_uFAST32) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_fast32_t v_xFAST32 = 42;
    if (sprintf(buffer, "%" PRIxFAST32, v_xFAST32) != 2 || strcmp(buffer, "2a"))
        return 1;
    uint_fast32_t v_XFAST32 = 42;
    if (sprintf(buffer, "%" PRIXFAST32, v_XFAST32) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            int_fast32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNdFAST32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            int_fast32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNiFAST32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_fast32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNoFAST32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_fast32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNuFAST32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_fast32_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNxFAST32, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    int_fast64_t v_dFAST64 = 42;
    if (sprintf(buffer, "%" PRIdFAST64, v_dFAST64) != 2 || strcmp(buffer, "42"))
        return 1;
    int_fast64_t v_iFAST64 = 42;
    if (sprintf(buffer, "%" PRIiFAST64, v_iFAST64) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_fast64_t v_oFAST64 = 42;
    if (sprintf(buffer, "%" PRIoFAST64, v_oFAST64) != 2 || strcmp(buffer, "52"))
        return 1;
    uint_fast64_t v_uFAST64 = 42;
    if (sprintf(buffer, "%" PRIuFAST64, v_uFAST64) != 2 || strcmp(buffer, "42"))
        return 1;
    uint_fast64_t v_xFAST64 = 42;
    if (sprintf(buffer, "%" PRIxFAST64, v_xFAST64) != 2 || strcmp(buffer, "2a"))
        return 1;
    uint_fast64_t v_XFAST64 = 42;
    if (sprintf(buffer, "%" PRIXFAST64, v_XFAST64) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            int_fast64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNdFAST64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            int_fast64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNiFAST64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_fast64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNoFAST64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_fast64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNuFAST64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uint_fast64_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNxFAST64, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    intmax_t v_dMAX = 42;
    if (sprintf(buffer, "%" PRIdMAX, v_dMAX) != 2 || strcmp(buffer, "42"))
        return 1;
    intmax_t v_iMAX = 42;
    if (sprintf(buffer, "%" PRIiMAX, v_iMAX) != 2 || strcmp(buffer, "42"))
        return 1;
    uintmax_t v_oMAX = 42;
    if (sprintf(buffer, "%" PRIoMAX, v_oMAX) != 2 || strcmp(buffer, "52"))
        return 1;
    uintmax_t v_uMAX = 42;
    if (sprintf(buffer, "%" PRIuMAX, v_uMAX) != 2 || strcmp(buffer, "42"))
        return 1;
    uintmax_t v_xMAX = 42;
    if (sprintf(buffer, "%" PRIxMAX, v_xMAX) != 2 || strcmp(buffer, "2a"))
        return 1;
    uintmax_t v_XMAX = 42;
    if (sprintf(buffer, "%" PRIXMAX, v_XMAX) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            intmax_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNdMAX, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            intmax_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNiMAX, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uintmax_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNoMAX, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uintmax_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNuMAX, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uintmax_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNxMAX, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    intptr_t v_dPTR = 42;
    if (sprintf(buffer, "%" PRIdPTR, v_dPTR) != 2 || strcmp(buffer, "42"))
        return 1;
    intptr_t v_iPTR = 42;
    if (sprintf(buffer, "%" PRIiPTR, v_iPTR) != 2 || strcmp(buffer, "42"))
        return 1;
    uintptr_t v_oPTR = 42;
    if (sprintf(buffer, "%" PRIoPTR, v_oPTR) != 2 || strcmp(buffer, "52"))
        return 1;
    uintptr_t v_uPTR = 42;
    if (sprintf(buffer, "%" PRIuPTR, v_uPTR) != 2 || strcmp(buffer, "42"))
        return 1;
    uintptr_t v_xPTR = 42;
    if (sprintf(buffer, "%" PRIxPTR, v_xPTR) != 2 || strcmp(buffer, "2a"))
        return 1;
    uintptr_t v_XPTR = 42;
    if (sprintf(buffer, "%" PRIXPTR, v_XPTR) != 2 || strcmp(buffer, "2A"))
        return 1;
    {
        struct {
            unsigned char before[8];
            intptr_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNdPTR, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            intptr_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("0x2a", "%" SCNiPTR, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uintptr_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("52", "%" SCNoPTR, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uintptr_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("42", "%" SCNuPTR, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    {
        struct {
            unsigned char before[8];
            uintptr_t value;
            unsigned char after[8];
        } slot;

        int k;
        memset(&slot, 90, sizeof(slot));
        slot.value = 0;
        if (sscanf("2a", "%" SCNxPTR, &slot.value) != 1 || slot.value != 42)
            return 2;
        for (k = 0; k < 8; k++)
            if (slot.before[k] != 90 || slot.after[k] != 90)
                return 3;
    }
    return 0;
}
