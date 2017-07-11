/* Copyright 2017 Rene Widera
 *
 * This file is part of libPMacc.
 *
 * libPMacc is free software: you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libPMacc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with libPMacc.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "pmacc_types.hpp"
#include "filter/Interface.hpp"
#include "mappings/threads/WorkerCfg.hpp"

#include <string>


namespace PMacc
{
namespace functor
{
namespace acc
{


    /** interface to combine a filter and a functor on the accelerator
     *
     * @tparam T_FilterOperator PMacc::filter::operators, type concatenate the
     *                          results of the filter
     * @tparam T_Filter PMacc::filter::Interface, type of the filter
     * @tparam T_Functor PMacc::functor::Interface, type of the functor
     */
    template<
        typename T_FilterOperator,
        typename T_Filter,
        typename T_Functor
    >
    struct Filtered :
        private T_Filter,
        private T_Functor
    {
        using Filter = T_Filter;
        using Functor = T_Functor;

        DINLINE Filtered(
            Filter const & filter,
            Functor const & functor
        ) :
            Filter( filter ),
            Functor( functor )
        {

        }

        /** execute the functor depending of the filter result
         *
         * Call the filter for each argument. If the combined result is true
         * the user functor is called.
         *
         * @param args arguments passed to the functor if the filter results of
         *             each argument evaluate to true when combined
         */
        template< typename ... T_Args >
        DINLINE auto operator( )(  T_Args && ... args )
        -> void
        {
            // call the filter on each argument and combine the results
            bool const combinedResult = T_FilterOperator{ }(
                ( *reinterpret_cast< Filter * >( this ) )( args ) ...
            );

            if( combinedResult )
                ( *reinterpret_cast< Functor * >( this ) )( args ... );
        }
    };

} // namespace acc

    /** combine a filter and a functor
     *
     * Creates a functor where each argument which is passed to
     * the accelerator instance is evaluated by the filter and if the
     * combined result is true the functor is executed.
     *
     * @tparam T_FilterOperator PMacc::filter::operators, type concatenate the
     *                          results of the filter
     * @tparam T_Filter PMacc::filter::Interface, type of the filter
     * @tparam T_Functor PMacc::functor::Interface, type of the functor
     */
    template<
        typename T_FilterOperator,
        typename T_Filter,
        typename T_Functor
    >
    struct Filtered;

    /** specialization of Filtered (with unary filter)
     *
     * This specialization can only be used if T_Filter is of the type PMacc::filter::Interface
     * and T_Functor is of the type PMacc::functor::Interface.
     * A unary filters means that each argument can only pass the same filter
     * check before its results are combined.
     */
    template<
        typename T_FilterOperator,
        typename T_Filter,
        typename T_Functor,
        uint32_t T_numFunctorArguments

    >
    struct Filtered<
        T_FilterOperator,
        filter::Interface<
            T_Filter,
            1u
        >,
        Interface<
            T_Functor,
            T_numFunctorArguments,
            void
        >
    > :
        private filter::Interface<
            T_Filter,
            1u
        >,
        Interface<
            T_Functor,
            T_numFunctorArguments,
            void
        >
    {

        template< typename ... T_Params >
        struct apply
        {
            using type = Filtered<
                T_FilterOperator,
                typename boost::mpl::apply<
                    T_Filter,
                    T_Params ...
                >::type,
                typename boost::mpl::apply<
                    T_Functor,
                    T_Params ...
                >::type
            >;
        };

        using Filter = filter::Interface<
            T_Filter,
            1u
        >;
        using Functor = Interface<
            T_Functor,
            T_numFunctorArguments,
            void
        >;

        template< typename DeferFunctor = Functor >
        HINLINE Filtered( uint32_t const currentStep ) :
            Filter( currentStep ),
            Functor( currentStep )
        {
        }


        /** create a filtered functor which can be used on the accelerator
         *
         * @tparam T_OffsetType type to describe the size of a domain
         * @tparam T_numWorkers number of workers
         *
         * @param domainOffset offset to the origin of the local domain
         *                     This can be e.g a supercell or cell offset and depends
         *                     of the context where the interface is specialized.
         * @param workerCfg configuration of the worker
         * @return accelerator instance of the filtered functor
         */
        template<
            typename T_OffsetType,
            uint32_t T_numWorkers
        >
        DINLINE auto
        operator( )(
            T_OffsetType const & domainOffset,
            mappings::threads::WorkerCfg< T_numWorkers > const & workerCfg
        )
        -> acc::Filtered<
            T_FilterOperator,
            decltype(
                std::declval< Filter >( )(
                    domainOffset,
                    workerCfg
                )
            ),
            decltype(
                std::declval< Functor >( )(
                    domainOffset,
                    workerCfg
                )
            )
        >
        {
            return acc::Filtered<
                T_FilterOperator,
                decltype(
                    std::declval< Filter >( )(
                        domainOffset,
                        workerCfg
                    )
                ),
                decltype(
                    std::declval< Functor >( )(
                        domainOffset,
                        workerCfg
                    )
                )
            >(
                ( *reinterpret_cast< Filter * >( this ) )(
                    domainOffset,
                    workerCfg
                ),
                ( *reinterpret_cast< Functor * >( this ) )(
                    domainOffset,
                    workerCfg
                )
            );
        }

        /** get name the of the filtered functor
         *
         * @return combination of the filter and functor name, the names are
         *         separated by an underscore `_`
         */
        HINLINE std::string
        getName( ) const
        {
            return Filter::getName( ) + std::string("_") + Functor::getName( );
        }
    };

} // namespace functor
} // namespace PMacc