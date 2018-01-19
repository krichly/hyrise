#pragma once

#include <boost/hana/fold.hpp>
#include <boost/hana/value.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/pair.hpp>

#include <memory>

// Include your encoded column file here!
#include "storage/dictionary_column.hpp"
#include "storage/run_length_column.hpp"
#include "storage/deprecated_dictionary_column.hpp"

#include "storage/encoding_type.hpp"

#include "utils/enum_constant.hpp"
#include "utils/template_type.hpp"

namespace opossum {

namespace hana = boost::hana;

/**
 * @brief Mapping of encoding types to columns
 *
 * Note: Add your encoded column class here!
 */
constexpr auto encoded_column_for_type =
    hana::make_map(hana::make_pair(enum_c<EncodingType, EncodingType::DeprecatedDictionary>,
                                   template_c<DeprecatedDictionaryColumn>),
                   hana::make_pair(enum_c<EncodingType, EncodingType::Dictionary>, template_c<DictionaryColumn>),
                   hana::make_pair(enum_c<EncodingType, EncodingType::RunLength>, template_c<RunLengthColumn>));

/**
 * @brief Resolves the type of an encoded column.
 *
 * Since encoded columns are immutable, the function accepts a constant reference.
 */
template <typename ColumnDataType, typename Functor>
void resolve_encoded_column_type(const BaseEncodedColumn& column, const Functor& functor) {
  hana::fold(encoded_column_for_type, false, [&](auto match_found, auto pair) {
    const auto encoding_type_c = hana::first(pair);
    const auto column_template_t = hana::second(pair);

    constexpr auto encoding_type = hana::value(encoding_type_c);

    if (!match_found && (encoding_type == column.encoding_type())) {
      const auto data_type_supported = encoding_supports_data_type(encoding_type_c, hana::type_c<ColumnDataType>);

      // clang-format off
      // Compile only for supported data types
      if constexpr(decltype(data_type_supported)::value) {
        using ColumnTemplateType = typename decltype(column_template_t)::type;
        using ColumnType = typename ColumnTemplateType::template _template<ColumnDataType>;
        functor(static_cast<const ColumnType&>(column));
      }
      // clang-format on

      return true;
    }

    return match_found;
  });
}

}  // namespace opossum