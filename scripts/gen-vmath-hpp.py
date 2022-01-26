TYS = {
    "uint": "uint32_t",
    "int": "int32_t",
    "float": "float",
    "bool": "bool",
}

def make_ctors(ty, ncomp):
    out = [
        f"  {ty}{ncomp}() = default;",
        f"  {ty}{ncomp}(const {ty}{ncomp}&) = default;",
        f"  {ty}{ncomp}({ty}{ncomp}&&) = default;",
        f"  {ty}{ncomp}& operator=(const {ty}{ncomp}&) = default;",
        f"  {ty}{ncomp}& operator=({ty}{ncomp}&&) = default;",
    ]

    if ncomp == 2:
        out += [f"  inline {ty}{ncomp}({TYS[ty]} x, {TYS[ty]} y) : x(x), y(y) {{}}"]
    if ncomp == 3:
        out += [f"  inline {ty}{ncomp}({TYS[ty]} x, {TYS[ty]} y, {TYS[ty]} z) : x(x), y(y), z(z) {{}}"]
    if ncomp == 4:
        out += [f"  inline {ty}{ncomp}({TYS[ty]} x, {TYS[ty]} y, {TYS[ty]} z, {TYS[ty]} w) : x(x), y(y), z(z), w(w) {{}}"]
    return out

def make_stream_ops(ty, ncomp):
    out = []
    if ncomp == 2:
        out += [f'  friend std::ostream& operator<<(std::ostream& out, const {ty}{ncomp}& a) {{ return out << "(" << a.x << ", " << a.y << ")"; }}']
    elif ncomp == 3:
        out += [f'  friend std::ostream& operator<<(std::ostream& out, const {ty}{ncomp}& a) {{ return out << "(" << a.x << ", " << a.y << ", " << a.z << ")"; }}']
    elif ncomp == 4:
        out += [f'  friend std::ostream& operator<<(std::ostream& out, const {ty}{ncomp}& a) {{ return out << "(" << a.x << ", " << a.y << ", " << a.z << ", " << a.w << ")"; }}']
    else:
        assert False
    return out

def make_casts(ty, ncomp):
    out = []
    for src_ty in TYS:
        if src_ty == ty: continue
        if ncomp == 2:
            out += [f"  {ty}{ncomp}(const struct {src_ty}{ncomp}& a);"]
        elif ncomp == 3:
            out += [f"  inline {ty}{ncomp}(const struct {src_ty}{ncomp}& a);"]
        elif ncomp == 4:
            out += [f"  inline {ty}{ncomp}(const struct {src_ty}{ncomp}& a);"]
        else:
            assert False
    return out

def make_cast_impls(ty, ncomp):
    out = []
    for src_ty in TYS:
        if src_ty == ty: continue
        if ncomp == 2:
            out += [f"  inline {ty}{ncomp}::{ty}{ncomp}(const {src_ty}{ncomp}& a) : x(({TYS[ty]})a.x), y(({TYS[ty]})a.y) {{}}"]
        elif ncomp == 3:
            out += [f"  inline {ty}{ncomp}::{ty}{ncomp}(const {src_ty}{ncomp}& a) : x(({TYS[ty]})a.x), y(({TYS[ty]})a.y), z(({TYS[ty]})a.z) {{}}"]
        elif ncomp == 4:
            out += [f"  inline {ty}{ncomp}::{ty}{ncomp}(const {src_ty}{ncomp}& a) : x(({TYS[ty]})a.x), y(({TYS[ty]})a.y), z(({TYS[ty]})a.z), w(({TYS[ty]})a.w) {{}}"]
        else:
            assert False
    return out

def make_vec_ty(ty, ncomp):
    out = []
    out += [f"struct {ty}{ncomp} {{"]
    if ncomp == 2:
        out += [f"  {TYS[ty]} x, y;"]
    elif ncomp == 3:
        out += [f"  {TYS[ty]} x, y, z;"]
    elif ncomp == 4:
        out += [f"  {TYS[ty]} x, y, z, w;"]
    else:
        assert False
    out += make_ctors(ty, ncomp)
    out += make_casts(ty, ncomp)
    out += make_stream_ops(ty, ncomp)
    out += ["};"]
    return out

def make_ty_unary_op(ty, ncomp, op):
    vty = f"{ty}{ncomp}"
    out = []
    if ncomp == 2:
        out += [f"inline {vty} operator{op}(const {vty}& a) {{ return {vty} {{ {op}a.x, {op}a.y }}; }}"]
    if ncomp == 3:
        out += [f"inline {vty} operator{op}(const {vty}& a) {{ return {vty} {{ {op}a.x, {op}a.y, {op}a.z }}; }}"]
    if ncomp == 4:
        out += [f"inline {vty} operator{op}(const {vty}& a) {{ return {vty} {{ {op}a.x, {op}a.y, {op}a.z, {op}a.w }}; }}"]
    return out

def make_ty_binary_op(ty, ncomp, op):
    vty = f"{ty}{ncomp}"
    out = []
    if ncomp == 2:
        out += [f"inline {vty} operator{op}(const {vty}& a, const {vty}& b) {{ return {vty} {{ a.x {op} b.x, a.y {op} b.y }}; }}"]
    if ncomp == 3:
        out += [f"inline {vty} operator{op}(const {vty}& a, const {vty}& b) {{ return {vty} {{ a.x {op} b.x, a.y {op} b.y, a.z {op} b.z }}; }}"]
    if ncomp == 4:
        out += [f"inline {vty} operator{op}(const {vty}& a, const {vty}& b) {{ return {vty} {{ a.x {op} b.x, a.y {op} b.y, a.z {op} b.z, a.w {op} b.w }}; }}"]
    return out

def make_ty_unary_intrinsic(ty, ncomp, name, op):
    vty = f"{ty}{ncomp}"
    out = []
    if ncomp == 2:
        out += [f"inline {vty} {name}(const {vty}& a, const {vty}& b) {{ return {vty} {{ {op}(a.x), {op}(a.y) }}; }}"]
    if ncomp == 3:
        out += [f"inline {vty} {name}(const {vty}& a, const {vty}& b) {{ return {vty} {{ {op}(a.x), {op}(a.y), {op}(a.z) }}; }}"]
    if ncomp == 4:
        out += [f"inline {vty} {name}(const {vty}& a, const {vty}& b) {{ return {vty} {{ {op}(a.x), {op}(a.y), {op}(a.z), {op}(a.w) }}; }}"]
    return out

def make_ty_binary_intrinsic(ty, ncomp, name, op):
    vty = f"{ty}{ncomp}"
    out = []
    if ncomp == 2:
        out += [f"inline {vty} {name}(const {vty}& a, const {vty}& b) {{ return {vty} {{ {op}(a.x, b.x), {op}(a.y, b.y) }}; }}"]
    if ncomp == 3:
        out += [f"inline {vty} {name}(const {vty}& a, const {vty}& b) {{ return {vty} {{ {op}(a.x, b.x), {op}(a.y, b.y), {op}(a.z, b.z) }}; }}"]
    if ncomp == 4:
        out += [f"inline {vty} {name}(const {vty}& a, const {vty}& b) {{ return {vty} {{ {op}(a.x, b.x), {op}(a.y, b.y), {op}(a.z, b.z), {op}(a.w, b.w) }}; }}"]
    return out

def make_ty_ops(ty, ncomp):
    out = []
    if "int" in ty or "float" in ty:
        out += make_ty_binary_op(ty, ncomp, "+")
        out += make_ty_binary_op(ty, ncomp, "-")
        out += make_ty_binary_op(ty, ncomp, "*")
        out += make_ty_binary_op(ty, ncomp, "/")
        if not "uint" in ty:
            out += make_ty_unary_op(ty, ncomp, "-")
        out += make_ty_binary_intrinsic(ty, ncomp, "min", "std::min")
        out += make_ty_binary_intrinsic(ty, ncomp, "max", "std::max")
    if "float" in ty:
        out += make_ty_binary_intrinsic(ty, ncomp, "atan2", "std::atan2f")
        out += make_ty_unary_intrinsic(ty, ncomp, "abs", "std::fabs")
        out += make_ty_unary_intrinsic(ty, ncomp, "floor", "std::floorf")
        out += make_ty_unary_intrinsic(ty, ncomp, "ceil", "std::ceilf")
        out += make_ty_unary_intrinsic(ty, ncomp, "round", "std::roundf")
        out += make_ty_unary_intrinsic(ty, ncomp, "sqrt", "std::sqrt")
        out += make_ty_unary_intrinsic(ty, ncomp, "trunc", "std::truncf")
        out += make_ty_unary_intrinsic(ty, ncomp, "sin", "std::sinf")
        out += make_ty_unary_intrinsic(ty, ncomp, "cos", "std::cosf")
        out += make_ty_unary_intrinsic(ty, ncomp, "tan", "std::tanf")
        out += make_ty_unary_intrinsic(ty, ncomp, "sinh", "std::sinhf")
        out += make_ty_unary_intrinsic(ty, ncomp, "cosh", "std::coshf")
        out += make_ty_unary_intrinsic(ty, ncomp, "tanh", "std::tanhf")
        out += make_ty_unary_intrinsic(ty, ncomp, "asin", "std::asinf")
        out += make_ty_unary_intrinsic(ty, ncomp, "acos", "std::acosf")
        out += make_ty_unary_intrinsic(ty, ncomp, "atan", "std::atanf")
        out += make_ty_unary_intrinsic(ty, ncomp, "asinh", "std::asinhf")
        out += make_ty_unary_intrinsic(ty, ncomp, "acosh", "std::acoshf")
        out += make_ty_unary_intrinsic(ty, ncomp, "atanh", "std::atanhf")
    if "int" in ty:
        out += make_ty_binary_op(ty, ncomp, "%")
        out += make_ty_binary_op(ty, ncomp, "&")
        out += make_ty_binary_op(ty, ncomp, "|")
        out += make_ty_binary_op(ty, ncomp, "^")
        out += make_ty_unary_op(ty, ncomp, "~")
    if "bool" in ty:
        out += make_ty_binary_op(ty, ncomp, "&&")
        out += make_ty_binary_op(ty, ncomp, "||")
        out += make_ty_unary_op(ty, ncomp, "!")
    return out

def make_ty_defs():
    out = []
    for ty in TYS:
        for ncomp in [2, 3, 4]:
            out += make_vec_ty(ty, ncomp)
            out += make_ty_ops(ty, ncomp)

    for ty in TYS:
        for ncomp in [2, 3, 4]:
            out += make_cast_impls(ty, ncomp)
    return out

OUT_FILE = '\n'.join(make_ty_defs())

with open("include/vmath.hpp", "w") as f:
    f.write('\n'.join([
        "// Vector math utilities.",
        "#pragma once",
        "#include <cstdint>",
        "#include <cmath>",
        "#include <ostream>",
        "",
        "namespace liong {",
        "namespace vmath {",
        ""
    ]))
    f.write(OUT_FILE + "\n")
    f.write('\n'.join([
        "} // namespace vmath",
        "} // namespace liong",
        "",
    ]))

