/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-02-04
 */

#pragma once

#ifndef SUIL_META_MAGIC
#define SUIL_META_MAGIC 0
#endif

#define MetaMagic(M) static_assert((M)==SUIL_META_MAGIC, "Invalid symbol usage");

/**
 * This is the macro parsed by suil tool chain
 * when parsing provide header file.
 *
 * @param TARGET This is the target token, it should
 * point to valid suil meta token.
 * @param __VA_ARGS__ these are the parameters parsed
 * to the target token
 */
#define $$(TARGET, ...) TARGET(SUIL_META_MAGIC, ##__VA_ARGS__)

#define REQUIRED(__, ...) __VA_ARGS__
#define OPTIONAL(__, ...) __VA_ARGS__

/**
 * Mark a file a meta compilable
 */
#define MetaFileGuard(__)  MetaMagic(__) \
static_assert(false, "this file is a meta file and should not be included as is, use file generated by meta tool")

/**
 * An empty
 */
struct EmptyStruct{};

/**
 * This is a token used to define a symbol in code. The tool
 * will look for these declarations.
 *
 * @example
 * ```
 * // This will declare a symbol named age in the symbol namespace
 * Symbol(age);
 * ```
 */
#define Symbol(__, NAME, ...) MetaMagic(__)

/**
 * The `Flags` macro is used by suil to generate an enum whose
 * fields are flags
 */
#define Flags(__, NAME, ...) MetaMagic(__) enum NAME

/**
 * This target is by the toolchain to generate meta attributes
 * @param NAME the name of the attribute to generate
 * @param __VA_ARGS__ this should be a json object (of identifier keys
 * and c++ literal values
 */
#define Attribute(__, NAME, ...) MetaMagic(__) struct NAME