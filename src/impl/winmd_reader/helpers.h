
namespace winmd::reader
{
    template <typename T>
    bool empty(std::pair<T, T> const& range) noexcept
    {
        return range.first == range.second;
    }

    template <typename T>
    std::size_t size(std::pair<T, T> const& range) noexcept
    {
        return range.second - range.first;
    }

    inline auto find(TypeRef const& type, Architecture arches)
    {
        if (type.ResolutionScope().type() != ResolutionScope::TypeRef)
        {
            return type.get_database().get_cache().find(type.TypeNamespace(), type.TypeName());
        }
        else
        {
            auto enclosing_type = find(type.ResolutionScope().TypeRef(), arches);
            if (!enclosing_type)
            {
                return TypeDef{};
            }
			if (arches == Architecture::None)
			{
				arches = Architecture::All;
			}
			auto pType = &enclosing_type;
			do
			{
				Architecture pType_arches = GetSupportedArchitectures(*pType);
				if (pType_arches == Architecture::None)
				{
					pType_arches = Architecture::All;
				}
				if (pType_arches & arches)
				{
					enclosing_type = *pType;
					break;
				}
			} while (pType = pType->next);
            auto const& nested_types = enclosing_type.get_cache().nested_types(enclosing_type);
            auto iter = std::find_if(nested_types.begin(), nested_types.end(),
                [name = type.TypeName()](TypeDef const& arg)
            {
                return name == arg.TypeName();
            });
            if (iter == nested_types.end())
            {
                return TypeDef{};
            }
            return *iter;
        }
    }

    inline auto find_required(TypeRef const& type)
    {
        if (type.ResolutionScope().type() != ResolutionScope::TypeRef)
        {
            return type.get_database().get_cache().find_required(type.TypeNamespace(), type.TypeName());
        }
        else
        {
            auto enclosing_type = find_required(type.ResolutionScope().TypeRef());
            auto const& nested_types = enclosing_type.get_cache().nested_types(enclosing_type);
            auto iter = std::find_if(nested_types.begin(), nested_types.end(),
                [name = type.TypeName()](TypeDef const& arg)
            {
                return name == arg.TypeName();
            });
            if (iter == nested_types.end())
            {
                impl::throw_invalid("Type '", enclosing_type.TypeName(), ".", type.TypeName(), "' could not be found");
            }
            return *iter;
        }
    }

    inline TypeDef find(coded_index<TypeDefOrRef> const& type, Architecture arches)
    {
        if (type.type() == TypeDefOrRef::TypeRef)
        {
            return find(type.TypeRef(), arches);
        }
        else if (type.type() == TypeDefOrRef::TypeDef)
        {
            return type.TypeDef();
        }
        else
        {
            XLANG_ASSERT(false);
            return {};
        }
    }

    inline TypeDef find_required(coded_index<TypeDefOrRef> const& type)
    {
        if (type.type() == TypeDefOrRef::TypeRef)
        {
            return find_required(type.TypeRef());
        }
        else if (type.type() == TypeDefOrRef::TypeDef)
        {
            return type.TypeDef();
        }
        else
        {
            XLANG_ASSERT(false);
            return {};
        }
    }

    inline bool is_const(ParamSig const& param)
    {
        auto is_type_const = [](auto&& type)
        {
            return type.TypeNamespace() == "System.Runtime.CompilerServices" && type.TypeName() == "IsConst";
        };

        for (auto const& cmod : param.CustomMod())
        {
            auto type = cmod.Type();

            if (type.type() == TypeDefOrRef::TypeDef)
            {
                if (is_type_const(type.TypeDef()))
                {
                    return true;
                }
            }
            else if (type.type() == TypeDefOrRef::TypeRef)
            {
                if (is_type_const(type.TypeRef()))
                {
                    return true;
                }
            }
        }

        return false;
    };

	template<class T>
	Architecture GetSupportedArchitectures(const T& type)
	{
		Architecture arches = Architecture::None;
		auto const attr = get_attribute(type, "Windows.Win32.Foundation.Metadata", "SupportedArchitectureAttribute");
		if (attr)
		{
			CustomAttributeSig attr_sig = attr.Value();
			FixedArgSig fixed_arg = attr_sig.FixedArgs()[0];
			ElemSig elem_sig = std::get<ElemSig>(fixed_arg.value);
			ElemSig::EnumValue enum_value = std::get<ElemSig::EnumValue>(elem_sig.value);
			auto const arch_flags = std::get<int32_t>(enum_value.value);
			arches = (Architecture)arch_flags;
		}
		return arches;
	}
}
