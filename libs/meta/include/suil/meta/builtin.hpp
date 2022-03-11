/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-11
 */

#pragma once
// The following include file is required by meta tool chain in order
// to pre-process the file. It should be the first processing token
// following the #pragma once token (if any is defined)
#include <suil/meta.hpp>
// The following guard is also required and should immediately follow
// the #include <suil/meta.hpp>
// $$(MetaFileGuard);

/**
 * Define a list of target on which attributes are applicable
 * @see targets attribute
 */
$$(Flags, Target) {
    Attributes,
    Class,
    Struct,
    Union,
    Enum,
    Field,
    Method
};

$$(Flags, Serializer) {
    Wire, Protobuf, Json
};

$$(Flags, Rpc) {
    WRpc, GRpc, JRpc
};

$$(Attribute, meta, {_alias = "meta", _of: Field|Method}) {
    $$(REQUIRED, int id);
};

$$(Attribute, meta, {_alias = "meta", _of: Field|Method}) {
    $$(REQUIRED, int id);
};



